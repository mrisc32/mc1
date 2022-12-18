----------------------------------------------------------------------------------------------------
-- Copyright (c) 2022 Marcus Geelnard
--
-- This software is provided 'as-is', without any express or implied warranty. In no event will the
-- authors be held liable for any damages arising from the use of this software.
--
-- Permission is granted to anyone to use this software for any purpose, including commercial
-- applications, and to alter it and redistribute it freely, subject to the following restrictions:
--
--  1. The origin of this software must not be misrepresented; you must not claim that you wrote
--     the original software. If you use this software in a product, an acknowledgment in the
--     product documentation would be appreciated but is not required.
--
--  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
--     being the original software.
--
--  3. This notice may not be removed or altered from any source distribution.
----------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------
-- This is a configurable SDRAM controller.
--
-- The controller supports burst transfers (e.g. reading/writing one 64-bit word per request from/to
-- a device with a 16-bit interface), and keeps track of the active row for each bank in order to
-- minimize the number of required PRECHARGE+ACTIVATE cycles. This minimizes the SDRAM protocol
-- overhead and allows data transfer rates close to the maximum bandwidth of the device. I.e. for
-- memory access patterns with reasonable temporal and spatial locality the average number of bits
-- transferred per clock cycle approaches the data interface width of the device.
--
-- Note: In order to reach maximum read performance, read requests should be pipelined. I.e. new
-- read requests should be sent to the controller as soon as o_busy is deasserted, even before o_ack
-- has been asserted for previous requests.
--
-- The design is based on the data sheet for ISSI IS42S16320D-7 (32Mx16), and tested with the same
-- device, but the configurability of the controller should make it suitable for many other devices.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity sdram_controller is
  generic (
    -- Clock frequency (in Hz).
    G_CLK_FREQ_HZ : integer;

    -- Host interface widths.
    G_DATA_WIDTH : integer := 32;
    G_ADDR_WIDTH : integer := 23;

    -- SDRAM interface widths.
    -- Default values are for ISSI IS42S16320D-7. Adjust to match your device.
    G_SDRAM_DQ_WIDTH : integer := 16;   -- Data width (io_sdram_dq).
    G_SDRAM_A_WIDTH : integer := 13;    -- Address width (o_sdram_a).
    G_SDRAM_BA_WIDTH : integer := 2;    -- Bank address width (o_sdram_ba).
    G_SDRAM_COL_WIDTH : integer := 9;   -- Column address width.
    G_SDRAM_ROW_WIDTH : integer := 13;  -- Row address width.

    -- SDRAM timing configuration (ns).
    -- Default values are for ISSI IS42S16320D-7. Adjust to match your device.
    G_T_DESL : real := 100_000.0;    -- Startup delay.
    G_T_RC : real := 60.0;           -- Row cycle time.
    G_T_RP : real := 15.0;           -- Precharge to activate delay.
    G_T_RCD : real := 15.0;          -- RAS to CAS delay.
    G_T_DPL : real := 14.0;          -- Write-to-precharge delay.
    G_T_MRD : real := 14.0;          -- Mode register cycle time.
    G_T_REF : real := 64_000_000.0;  -- Refresh cycle time.

    -- CAS latency (cycles).
    -- Usually 2 or 3, where the longer latency is required for higher clock frequencies (e.g. over
    -- 133 MHz for some memory models).
    G_CAS_LATENCY : integer := 2
  );
  port (
    i_rst : in std_logic;
    i_clk : in std_logic;

    -- Host interface.
    i_adr : in std_logic_vector(G_ADDR_WIDTH-1 downto 0);
    i_dat_w : in std_logic_vector(G_DATA_WIDTH-1 downto 0);
    i_we : in std_logic;
    i_sel : in std_logic_vector(G_DATA_WIDTH/8-1 downto 0);
    i_req : in std_logic;
    o_busy : out std_logic;
    o_ack : out std_logic;
    o_dat : out std_logic_vector(G_DATA_WIDTH-1 downto 0);

    -- SDRAM interface.
    o_sdram_a : out std_logic_vector(G_SDRAM_A_WIDTH-1 downto 0);
    o_sdram_ba : out std_logic_vector(G_SDRAM_BA_WIDTH-1 downto 0);
    io_sdram_dq : inout std_logic_vector(G_SDRAM_DQ_WIDTH-1 downto 0);
    o_sdram_cke : out std_logic;
    o_sdram_cs_n : out std_logic;
    o_sdram_ras_n : out std_logic;
    o_sdram_cas_n : out std_logic;
    o_sdram_we_n : out std_logic;
    o_sdram_dqm : out std_logic_vector(G_SDRAM_DQ_WIDTH/8-1 downto 0)
  );

  -- Use I/O flip-flops for the SDRAM output signals.
  attribute useioff : boolean;
  attribute useioff of o_sdram_a : signal is true;
  attribute useioff of o_sdram_ba : signal is true;
  attribute useioff of o_sdram_cke : signal is true;
  attribute useioff of o_sdram_cs_n : signal is true;
  attribute useioff of o_sdram_ras_n : signal is true;
  attribute useioff of o_sdram_cas_n : signal is true;
  attribute useioff of o_sdram_we_n : signal is true;
  attribute useioff of o_sdram_dqm : signal is true;
end sdram_controller;

architecture rtl of sdram_controller is
  -- Helper function: log2 for integer numbers (rounding up).
  function ilog2(n : integer) return integer is
  begin
    return integer(ceil(log2(real(n))));
  end;

  -- Burst configuration.
  constant C_BURST_LENGTH : integer := G_DATA_WIDTH / G_SDRAM_DQ_WIDTH;
  constant C_BURST_TYPE : std_logic := '0';        -- 0: sequential, 1: interleaved
  constant C_WRITE_BURST_MODE : std_logic := '0';  -- 0: burst, 1: single

  -- Convert the timing constraints to clock cycles.
  constant C_CLK_PERIOD_NS : real := 1_000_000_000.0 / real(G_CLK_FREQ_HZ);
  constant C_DESL_CYCLES : integer := integer(ceil(G_T_DESL / C_CLK_PERIOD_NS));
  constant C_RC_CYCLES : integer := integer(ceil(G_T_RC / C_CLK_PERIOD_NS));
  constant C_RP_CYCLES : integer := integer(ceil(G_T_RP / C_CLK_PERIOD_NS));
  constant C_RCD_CYCLES : integer := integer(ceil(G_T_RCD / C_CLK_PERIOD_NS));
  constant C_MRD_CYCLES : integer := integer(ceil(G_T_MRD / C_CLK_PERIOD_NS));
  constant C_DPL_CYCLES : integer := integer(ceil(G_T_DPL / C_CLK_PERIOD_NS));

  -- The interval between auto-refresh must be at most Tref / NumRows.
  constant C_REFRESH_INTERVAL_NS : real := G_T_REF / real(2 ** G_SDRAM_ROW_WIDTH);
  constant C_REFRESH_INTERVAL_CYCLES : integer := integer(floor(C_REFRESH_INTERVAL_NS / C_CLK_PERIOD_NS));

  -- States.
  type T_STATE is (ST_INIT,
                   ST_INIT_DESELECT,
                   ST_INIT_REFRESH1,
                   ST_INIT_REFRESH2,
                   ST_INIT_SET_MODE,
                   ST_SERVICE_REQUEST,
                   ST_ACTIVE,
                   ST_DELAYED_WRITE,
                   ST_BURST,
                   ST_DELAYED_PRECHARGE_ONE_BANK,
                   ST_DELAYED_PRECHARGE_ALL_BANKS,
                   ST_PRECHARGE,
                   ST_REFRESH);
  signal s_state : T_STATE;
  signal s_next_state : T_STATE;
  signal s_return_state : T_STATE;
  signal s_next_return_state : T_STATE;

  -- Commands.
  subtype T_CMD is std_logic_vector(3 downto 0);
  constant C_CMD_MRS  : T_CMD := "0000";  -- Mode register set
  constant C_CMD_REF  : T_CMD := "0001";  -- Auto-refresh
  constant C_CMD_PRE  : T_CMD := "0010";  -- Precharge select bank
  constant C_CMD_ACT  : T_CMD := "0011";  -- Bank activate
  constant C_CMD_WR   : T_CMD := "0100";  -- Write
  constant C_CMD_RD   : T_CMD := "0101";  -- Read
  constant C_CMD_BST  : T_CMD := "0110";  -- Burst stop
  constant C_CMD_NOP  : T_CMD := "0111";  -- No operation
  constant C_CMD_DESL : T_CMD := "1---";  -- Device deselect
  signal s_cmd : T_CMD;
  signal s_next_cmd : T_CMD;
  signal s_return_cmd : T_CMD;
  signal s_next_return_cmd : T_CMD;

  -- Counters.
  function get_max_state_cycle_count return integer is
    variable v_max_cycles : integer;
  begin
    v_max_cycles := C_DESL_CYCLES;
    if C_RC_CYCLES > v_max_cycles then v_max_cycles := C_RC_CYCLES; end if;
    if C_RP_CYCLES > v_max_cycles then v_max_cycles := C_RP_CYCLES; end if;
    if C_RCD_CYCLES > v_max_cycles then v_max_cycles := C_RCD_CYCLES; end if;
    if C_MRD_CYCLES > v_max_cycles then v_max_cycles := C_MRD_CYCLES; end if;
    return v_max_cycles;
  end;
  constant C_MAX_STATE_CYCLE_CNT : integer := get_max_state_cycle_count;
  constant C_MAX_REFRESH_CYCLE_CNT : integer := C_REFRESH_INTERVAL_CYCLES + 1;
  constant C_MAX_DPL_CYCLE_CNT : integer := C_DPL_CYCLES;
  constant C_MAX_READ_CYCLE_CNT : integer := G_CAS_LATENCY + C_BURST_LENGTH;
  signal s_state_cycle_cnt : integer range 0 to C_MAX_STATE_CYCLE_CNT;
  signal s_refresh_cycle_cnt : integer range 0 to C_MAX_REFRESH_CYCLE_CNT;
  signal s_dpl_cycle_cnt : integer range 0 to C_MAX_DPL_CYCLE_CNT;
  signal s_read_cycle_cnt : integer range 0 to C_MAX_READ_CYCLE_CNT;
  signal s_rd_burst_cnt : integer range 0 to C_BURST_LENGTH-1;
  signal s_wr_burst_cnt : integer range 0 to C_BURST_LENGTH-1;

  -- Latched request signals.
  signal s_latched_adr : std_logic_vector(G_ADDR_WIDTH-1 downto 0);
  signal s_latched_dat_w : std_logic_vector(G_DATA_WIDTH-1 downto 0);
  signal s_latched_we : std_logic;
  signal s_latched_sel_n : std_logic_vector(G_DATA_WIDTH/8-1 downto 0);

  -- Current (latched or unlatched) request signals.
  signal s_adr : std_logic_vector(G_ADDR_WIDTH-1 downto 0);
  signal s_we : std_logic;

  -- Read buffer.
  signal s_dat : std_logic_vector(G_DATA_WIDTH-1 downto 0);

  -- A delay line for keeping track of read responses.
  -- Note: We add one extra cycle delay due to the SDRAM clock phase diff.
  constant C_READ_DLY_LEN : integer := G_CAS_LATENCY + 1;
  signal s_read_delay_line : std_logic_vector(C_READ_DLY_LEN-1 downto 0);
  signal s_read_burst_data_start : std_logic;

  -- Decoded address.
  signal s_col : std_logic_vector(G_SDRAM_COL_WIDTH-1 downto 0);
  signal s_row : std_logic_vector(G_SDRAM_ROW_WIDTH-1 downto 0);
  signal s_bank : std_logic_vector(G_SDRAM_BA_WIDTH-1 downto 0);

  -- Control signals.
  signal s_busy : std_logic;
  signal s_ack : std_logic;
  signal s_start_req : std_logic;
  signal s_time_for_refresh : std_logic;
  signal s_next_time_for_refresh : std_logic;
  signal s_precharge_all_banks : std_logic;

  -- Per bank state.
  constant C_NUM_BANKS : integer := 2**G_SDRAM_BA_WIDTH;
  type T_BANK_STATE is record
    row : std_logic_vector(G_SDRAM_ROW_WIDTH-1 downto 0);
    active : std_logic;
  end record;
  type T_BANK_STATE_ARRAY is array (0 to C_NUM_BANKS-1) of T_BANK_STATE;
  signal s_bank_state : T_BANK_STATE_ARRAY;

  -- SDRAM DQ in/out signals (mapped to/from io_sdram_dq).
  signal s_dq_in : std_logic_vector(G_SDRAM_DQ_WIDTH-1 downto 0);
  signal s_dq_out : std_logic_vector(G_SDRAM_DQ_WIDTH-1 downto 0);
  signal s_dq_out_en : std_logic;

  -- Use I/O flip-flops for the SDRAM data in/out signals.
  attribute useioff of s_dq_in : signal is true;
  attribute useioff of s_dq_out : signal is true;

  -- Convert the precharge setting (all banks/single bank) to a signal suitable for o_sdram_a.
  function pre2sdram_a(precharge_all_banks : std_logic) return std_logic_vector is
    variable a : std_logic_vector(G_SDRAM_A_WIDTH-1 downto 0);
  begin
    a(9 downto 0) := (others => '0');
    a(10) := precharge_all_banks;
    a(G_SDRAM_A_WIDTH-1 downto 11) := (others => '0');
    return a;
  end;

  -- Convert the mode register setting to a signal suitable for o_sdram_a.
  function mode2sdram_a return std_logic_vector is
    variable a : std_logic_vector(G_SDRAM_A_WIDTH-1 downto 0);
  begin
    a(2 downto 0) := std_logic_vector(to_unsigned(ilog2(C_BURST_LENGTH), 3));
    a(3) := C_BURST_TYPE;
    a(6 downto 4) := std_logic_vector(to_unsigned(ilog2(G_CAS_LATENCY), 3));
    a(8 downto 7) := "00";
    a(9) := C_WRITE_BURST_MODE;
    a(G_SDRAM_A_WIDTH-1 downto 10) := (others => '0');
    return a;
  end;

  -- Convert a ROW address to a signal suitable for o_sdram_a.
  function row2sdram_a(row : std_logic_vector) return std_logic_vector is
    variable a : std_logic_vector(G_SDRAM_A_WIDTH-1 downto 0);
  begin
    a(G_SDRAM_ROW_WIDTH-1 downto 0) := row;
    a(G_SDRAM_A_WIDTH-1 downto G_SDRAM_ROW_WIDTH) := (others => '0');
    return a;
  end;

  -- Convert a COL address to a signal suitable for o_sdram_a.
  function col2sdram_a(col : std_logic_vector) return std_logic_vector is
    variable a : std_logic_vector(G_SDRAM_A_WIDTH-1 downto 0);
  begin
    a(9 downto 0) := col(9 downto 0);
    a(10) := '0';  -- 0 = no auto-precharge
    a(G_SDRAM_COL_WIDTH downto 11) := col(G_SDRAM_COL_WIDTH-1 downto 10);
    a(G_SDRAM_A_WIDTH-1 downto G_SDRAM_COL_WIDTH+1) := (others => '0');
    return a;
  end;
begin
  --------------------------------------------------------------------------------------------------
  -- Host interface.
  --------------------------------------------------------------------------------------------------

  -- Is this the start of a new request?
  s_start_req <= i_req and not s_busy;

  -- Latch the request.
  process (i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_latched_adr <= (others => '0');
      s_latched_we <= '0';
    elsif rising_edge(i_clk) then
      if s_start_req = '1' then
        s_latched_adr <= i_adr;
        s_latched_we <= i_we;
      end if;
    end if;
  end process;

  -- Select unlatched or latched input signals.
  process (all)
  begin
    if s_start_req = '1' then
      -- Use un-latched input signals during the first request cycle.
      s_adr <= i_adr;
      s_we <= i_we;
    else
      -- Use latched input signals during the rest of the cycles.
      s_adr <= s_latched_adr;
      s_we <= s_latched_we;
    end if;
  end process;

  -- Decode/split the address into col/row/bank.
  -- Note: We adjust the incoming address to the SDRAM address space (e.g. from 32-bit word
  -- addressing to 16-bit word addressing) by padding zeros in the least significant SDRAM address
  -- bits.
  process (s_adr)
    constant C_ADR_PAD_BITS : integer := ilog2(G_DATA_WIDTH / G_SDRAM_DQ_WIDTH);
    constant C_ADR_PAD : std_logic_vector(C_ADR_PAD_BITS-1 downto 0) := (others => '0');
    variable v_adr : std_logic_vector(G_ADDR_WIDTH+C_ADR_PAD_BITS-1 downto 0);
  begin
    v_adr := s_adr & C_ADR_PAD;
    s_col <= v_adr(G_SDRAM_COL_WIDTH-1 downto 0);
    s_bank <= v_adr(G_SDRAM_COL_WIDTH+G_SDRAM_BA_WIDTH-1 downto G_SDRAM_COL_WIDTH);
    s_row <= v_adr(G_SDRAM_COL_WIDTH+G_SDRAM_BA_WIDTH+G_SDRAM_ROW_WIDTH-1 downto G_SDRAM_COL_WIDTH+G_SDRAM_BA_WIDTH);
  end process;

  -- Responses to the host interface.
  o_busy <= s_busy;
  o_ack <= s_ack;
  o_dat <= s_dat;


  --------------------------------------------------------------------------------------------------
  -- SDRAM interface.
  --------------------------------------------------------------------------------------------------

  -- Since the io_sdram_dq signal is an inout signal, we need to use constructs that map well to
  -- FPGA I/O buffers.

  -- Sample the input DQ signal.
  process (i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_dq_in <= (others => '0');
    elsif rising_edge(i_clk) then
      s_dq_in <= io_sdram_dq;
    end if;
  end process;

  -- This should infer an IOBUF.
  io_sdram_dq <= s_dq_out when s_dq_out_en = '1' else (others => 'Z');

  -- Enable the SDRAM clock as soon as we exit reset.
  process (i_rst, i_clk)
  begin
    if i_rst = '1' then
      o_sdram_cke <= '0';
    elsif rising_edge(i_clk) then
      o_sdram_cke <= '1';
    end if;
  end process;

  -- Send the current command to the SDRAM control signals.
  (o_sdram_cs_n, o_sdram_ras_n, o_sdram_cas_n, o_sdram_we_n) <= s_cmd;

  -- Set the SDRAM bank and address lines depending on which command we are executing.
  process (i_rst, i_clk)
  begin
    if i_rst = '1' then
      o_sdram_ba <= (others => '0');
      o_sdram_a <= (others => '0');
    elsif rising_edge(i_clk) then
      -- Set the bank.
      case s_next_cmd is
        when C_CMD_ACT | C_CMD_WR | C_CMD_RD =>
          o_sdram_ba <= s_bank;
        when C_CMD_PRE =>
          if s_precharge_all_banks = '1' then
            o_sdram_ba <= (others => '0');
          else
            o_sdram_ba <= s_bank;
          end if;
        when others =>
          o_sdram_ba <= (others => '0');
      end case;

      -- Set the address.
      case s_next_cmd is
        when C_CMD_PRE =>
          o_sdram_a <= pre2sdram_a(s_precharge_all_banks);
        when C_CMD_MRS =>
          o_sdram_a <= mode2sdram_a;
        when C_CMD_ACT =>
          o_sdram_a <= row2sdram_a(s_row);
        when C_CMD_WR | C_CMD_RD =>
          o_sdram_a <= col2sdram_a(s_col);
        when others =>
          o_sdram_a <= (others => '0');
      end case;
    end if;
  end process;


  --------------------------------------------------------------------------------------------------
  -- Counters that control the FSM.
  --------------------------------------------------------------------------------------------------

  process (i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_state_cycle_cnt <= 0;
      s_refresh_cycle_cnt <= C_REFRESH_INTERVAL_CYCLES;
    elsif rising_edge(i_clk) then
      -- The state cycle counter keeps track of how many cycles have been spent in the current FSM
      -- state. We restart the state cycle counter whenever we have a state change.
      if s_next_state /= s_state then
        s_state_cycle_cnt <= 0;
      elsif s_state_cycle_cnt /= C_MAX_STATE_CYCLE_CNT then
        s_state_cycle_cnt <= s_state_cycle_cnt + 1;
      end if;

      -- The refresh counter is free running as we want it to trigger a refresh (C_CMD_REF) at
      -- regular intervals, regardless of what the state machine has been up to.
      if s_refresh_cycle_cnt /= 0 then
        s_refresh_cycle_cnt <= s_refresh_cycle_cnt - 1;
      else
        s_refresh_cycle_cnt <= C_REFRESH_INTERVAL_CYCLES;
      end if;
    end if;
  end process;

  -- Determine if it's time to start a refresh cycle.
  s_next_time_for_refresh <=
      '1' when s_refresh_cycle_cnt = 0 else
      '0' when s_next_cmd <= C_CMD_REF else
      s_time_for_refresh;

  process (i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_time_for_refresh <= '0';
    elsif rising_edge(i_clk) then
      s_time_for_refresh <= s_next_time_for_refresh;
    end if;
  end process;


  --------------------------------------------------------------------------------------------------
  -- State machine (FSM).
  --------------------------------------------------------------------------------------------------

  -- State transition logic (unregistered).
  process (all)
    variable v_bank_state : T_BANK_STATE;
  begin
    -- Default state variables if not defined by a state transition.
    s_next_state <= s_state;
    s_next_cmd <= C_CMD_NOP;
    s_next_return_state <= s_return_state;
    s_next_return_cmd <= s_return_cmd;

    -- By default we will not precharge all banks.
    s_precharge_all_banks <= '0';

    case s_state is
      ---------------------------------------------------------------------------------------------
      -- The initialization sequence follows the documentation of the ISSI IS42S16320D:
      -- "A 100μs delay is required prior to issuing any command other than a COMMAND INHIBIT or a
      --  NOP. The COMMAND INHIBIT or NOP may be applied during the 100us period and should
      --  continue at least through the end of the period.
      --  With at least one COMMAND INHIBIT or NOP command having been applied, a PRECHARGE command
      --  should be applied once the 100μs delay has been satisfied. All banks must be precharged.
      --  This will leave all banks in an idle state after which at least two AUTO REFRESH cycles
      --  must be performed. After the AUTO REFRESH cycles are complete, the SDRAM is then ready
      --  for mode register programming.
      --  The mode register should be loaded prior to applying any operational command because it
      --  will power up in an unknown state."
      ---------------------------------------------------------------------------------------------
      when ST_INIT =>
        s_next_state <= ST_INIT_DESELECT;
        s_next_cmd <= C_CMD_DESL;

      when ST_INIT_DESELECT =>
        if s_state_cycle_cnt = C_DESL_CYCLES then
          s_next_state <= ST_PRECHARGE;
          s_next_cmd <= C_CMD_PRE;
          s_next_return_state <= ST_INIT_REFRESH1;
          s_next_return_cmd <= C_CMD_REF;
          s_precharge_all_banks <= '1';
        end if;

      when ST_INIT_REFRESH1 =>
        if s_state_cycle_cnt = C_RC_CYCLES then
          s_next_state <= ST_INIT_REFRESH2;
          s_next_cmd <= C_CMD_REF;
        end if;

      when ST_INIT_REFRESH2 =>
        if s_state_cycle_cnt = C_RC_CYCLES then
          s_next_state <= ST_INIT_SET_MODE;
          s_next_cmd <= C_CMD_MRS;
        end if;

      when ST_INIT_SET_MODE =>
        if s_state_cycle_cnt = C_MRD_CYCLES then
          s_next_state <= ST_SERVICE_REQUEST;
        end if;

      ---------------------------------------------------------------------------------------------
      -- From here on we can service new memory read/write requests.
      ---------------------------------------------------------------------------------------------
      when ST_SERVICE_REQUEST =>
        if s_time_for_refresh = '1' then
          -- Do a PRE (all banks) followed by REF.
          if s_dpl_cycle_cnt > 1 then
            -- Handle write-to-precharge (+Tdpl delay).
            s_next_state <= ST_DELAYED_PRECHARGE_ALL_BANKS;
            s_next_return_state <= ST_REFRESH;
            s_next_return_cmd <= C_CMD_REF;
          else
            s_next_state <= ST_PRECHARGE;
            s_next_cmd <= C_CMD_PRE;
            s_precharge_all_banks <= '1';
            s_next_return_state <= ST_REFRESH;
            s_next_return_cmd <= C_CMD_REF;
          end if;
        else
          if i_req = '1' then
            -- Check the state of this bank.
            v_bank_state := s_bank_state(to_integer(unsigned(s_bank)));
            if v_bank_state.active = '1' and v_bank_state.row = s_row then
              -- Send the WR/RD command immediately if we're in an already active row.
              if i_we = '1' and s_read_cycle_cnt > 1 then
                -- Handle read-to-write (+CAS latency delay).
                s_next_state <= ST_DELAYED_WRITE;
              else
                if i_we = '1' then
                  s_next_cmd <= C_CMD_WR;
                else
                  s_next_cmd <= C_CMD_RD;
                end if;
                if C_BURST_LENGTH >= 2 then
                  s_next_state <= ST_BURST;
                else
                  s_next_state <= ST_SERVICE_REQUEST;
                end if;
              end if;
            elsif v_bank_state.active = '0' then
              -- Activate a new row for this bank, but skip PRE since no row was active.
              s_next_state <= ST_ACTIVE;
              s_next_cmd <= C_CMD_ACT;
            else
              -- Do the full PRE+ACT dance if we need to change from one active row to another.
              if s_dpl_cycle_cnt > 1 then
                -- Handle write-to-precharge (+Tdpl delay).
                s_next_state <= ST_DELAYED_PRECHARGE_ONE_BANK;
                s_next_return_state <= ST_ACTIVE;
                s_next_return_cmd <= C_CMD_ACT;
              else
                -- Issue PRE with a request to do ACT after that.
                s_next_state <= ST_PRECHARGE;
                s_next_cmd <= C_CMD_PRE;
                s_next_return_state <= ST_ACTIVE;
                s_next_return_cmd <= C_CMD_ACT;
              end if;
            end if;
          end if;
        end if;

      when ST_ACTIVE =>
        if s_state_cycle_cnt = C_RCD_CYCLES then
          if s_latched_we = '1' and s_read_cycle_cnt > 1 then
            -- Handle read-to-write (+CAS latency delay).
            s_next_state <= ST_DELAYED_WRITE;
          else
            if s_latched_we = '1' then
              s_next_cmd <= C_CMD_WR;
            else
              s_next_cmd <= C_CMD_RD;
            end if;
            if C_BURST_LENGTH >= 2 then
              s_next_state <= ST_BURST;
            else
              s_next_state <= ST_SERVICE_REQUEST;
            end if;
          end if;
        end if;

      when ST_DELAYED_WRITE =>
        if s_read_cycle_cnt <= 1 then
          s_next_cmd <= C_CMD_WR;
          if C_BURST_LENGTH >= 2 then
            s_next_state <= ST_BURST;
          else
            s_next_state <= ST_SERVICE_REQUEST;
          end if;
        end if;

      when ST_BURST =>
        if s_state_cycle_cnt = C_BURST_LENGTH-2 then
          s_next_state <= ST_SERVICE_REQUEST;
        end if;

      when ST_DELAYED_PRECHARGE_ONE_BANK =>
        -- Note: States that invoke this state are expected to define s_next_return_*.
        if s_dpl_cycle_cnt <= 1 then
          s_next_state <= ST_PRECHARGE;
          s_next_cmd <= C_CMD_PRE;
        end if;

      when ST_DELAYED_PRECHARGE_ALL_BANKS =>
        -- Note: States that invoke this state are expected to define s_next_return_*.
        if s_dpl_cycle_cnt <= 1 then
          s_next_state <= ST_PRECHARGE;
          s_next_cmd <= C_CMD_PRE;
          s_precharge_all_banks <= '1';
        end if;

      when ST_PRECHARGE =>
        -- Note: States that invoke this state are expected to define s_next_return_*.
        if s_state_cycle_cnt = C_RP_CYCLES then
          s_next_state <= s_return_state;
          s_next_cmd <= s_return_cmd;
        end if;

      ---------------------------------------------------------------------------------------------
      -- Auto-refresh cycle, triggered at regular intervals.
      -- Note: The auto-refresh command (REF) must be preceded by a precharge of all banks.
      ---------------------------------------------------------------------------------------------
      when ST_REFRESH =>
        if s_state_cycle_cnt = C_RC_CYCLES then
          s_next_state <= ST_SERVICE_REQUEST;
        end if;
    end case;
  end process;

  -- State registers.
  process (i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_state <= ST_INIT;
      s_cmd <= C_CMD_NOP;
      s_return_state <= ST_INIT;
      s_return_cmd <= C_CMD_NOP;
      s_busy <= '1';
    elsif rising_edge(i_clk) then
      s_state <= s_next_state;
      s_cmd <= s_next_cmd;
      s_return_state <= s_next_return_state;
      s_return_cmd <= s_next_return_cmd;

      -- Will the controller be busy (unable to accept requests) during the next cycle?
      if s_next_state = ST_SERVICE_REQUEST and s_next_time_for_refresh = '0' then
        s_busy <= '0';
      else
        s_busy <= '1';
      end if;
    end if;
  end process;


  --------------------------------------------------------------------------------------------------
  -- Bank state logic.
  --------------------------------------------------------------------------------------------------

  process (i_rst, i_clk)
    variable v_active : std_logic;
    variable v_all_banks : std_logic;
  begin
    if i_rst = '1' then
      for k in 0 to C_NUM_BANKS-1 loop
        s_bank_state(k).row <= (others => '0');
        s_bank_state(k).active <= '0';
      end loop;
    elsif rising_edge(i_clk) then
      if s_next_cmd = C_CMD_ACT or s_next_cmd = C_CMD_PRE then
        -- Are we activating or deactivating?
        v_active := '0';
        if s_next_cmd = C_CMD_ACT then
          v_active := '1';
        end if;

        -- All banks or just a single one?
        v_all_banks := '0';
        if s_next_cmd = C_CMD_PRE and s_precharge_all_banks = '1' then
          v_all_banks := '1';
        end if;

        for k in 0 to C_NUM_BANKS-1 loop
          if k = to_integer(unsigned(s_bank)) or v_all_banks = '1' then
            s_bank_state(k).row <= s_row;
            s_bank_state(k).active <= v_active;
          end if;
        end loop;
      end if;
    end if;
  end process;


  --------------------------------------------------------------------------------------------------
  -- Read & write burst logic.
  --------------------------------------------------------------------------------------------------

  -- Read request keeping-track-of-CAS-latency thingy...
  process (i_rst, i_clk)
    variable v_rd_started : std_logic;
  begin
    if i_rst = '1' then
      s_read_delay_line <= (others => '0');
      s_read_cycle_cnt <= 0;
    elsif rising_edge(i_clk) then
      -- Was a new read operation started in this cycle?
      if s_next_cmd = C_CMD_RD then
        v_rd_started := '1';
        s_read_cycle_cnt <= C_MAX_READ_CYCLE_CNT;
      else
        v_rd_started := '0';
        if s_read_cycle_cnt /= 0 then
          s_read_cycle_cnt <= s_read_cycle_cnt - 1;
        end if;
      end if;

      -- Shift in new read start events into the delay line.
      s_read_delay_line <= s_read_delay_line(C_READ_DLY_LEN-2 downto 0) & v_rd_started;
    end if;
  end process;

  -- The MSB of the read delay line is asserted when the first data word of a burst arrives
  -- from SDRAM.
  s_read_burst_data_start <= s_read_delay_line(C_READ_DLY_LEN-1);

  process (i_rst, i_clk)
    variable v_read_done : std_logic;
    variable v_write_done : std_logic;
  begin
    if i_rst = '1' then
      -- Read signals.
      s_dat <= (others => '0');
      s_rd_burst_cnt <= C_BURST_LENGTH-1;

      -- Write signals.
      s_latched_dat_w <= (others => '0');
      s_latched_sel_n <= (others => '0');
      s_dq_out_en <= '0';
      s_dq_out <= (others => '0');
      o_sdram_dqm <= (others => '0');
      s_dpl_cycle_cnt <= 0;
      s_wr_burst_cnt <= C_BURST_LENGTH-1;

      s_ack <= '0';
    elsif rising_edge(i_clk) then
      ------------------------------------------------------------
      -- Read the next sub-word as it's bursted from the SDRAM.
      ------------------------------------------------------------
      -- Shift in the data from the SDRAM.
      s_dat(G_DATA_WIDTH-G_SDRAM_DQ_WIDTH-1 downto 0) <= s_dat(G_DATA_WIDTH-1 downto G_SDRAM_DQ_WIDTH);
      s_dat(G_DATA_WIDTH-1 downto G_DATA_WIDTH-G_SDRAM_DQ_WIDTH) <= s_dq_in;

      -- Was this the final sub-word?
      if (s_read_burst_data_start = '1' and C_BURST_LENGTH = 1) or
         (s_rd_burst_cnt = (C_BURST_LENGTH - 2)) then
        v_read_done := '1';
      else
        v_read_done := '0';
      end if;

      -- Update the read burst counter.
      if s_read_burst_data_start = '1' then
        s_rd_burst_cnt <= 0;
      elsif s_rd_burst_cnt < (C_BURST_LENGTH-1) then
        s_rd_burst_cnt <= s_rd_burst_cnt + 1;
      end if;

      -------------------------------------------------------------------------------
      -- Write the next sub-word from the write buffer to the SDRAM.
      -- We also keep track of the Tdpl counter here (i.e. write-to-precharge delay).
      -------------------------------------------------------------------------------
      -- Shift the write data & mask in pace with the burst.
      if s_start_req = '1' then
        if s_next_cmd /= C_CMD_WR then
          s_latched_dat_w <= i_dat_w;
          s_latched_sel_n <= not i_sel;
        else
          s_latched_dat_w(G_DATA_WIDTH-1 downto G_DATA_WIDTH-G_SDRAM_DQ_WIDTH) <= (others => '0');
          s_latched_dat_w(G_DATA_WIDTH-G_SDRAM_DQ_WIDTH-1 downto 0) <= i_dat_w(G_DATA_WIDTH-1 downto G_SDRAM_DQ_WIDTH);
          s_latched_sel_n(G_DATA_WIDTH/8-1 downto (G_DATA_WIDTH-G_SDRAM_DQ_WIDTH)/8) <= (others => '1');
          s_latched_sel_n((G_DATA_WIDTH-G_SDRAM_DQ_WIDTH)/8-1 downto 0) <= not i_sel(G_DATA_WIDTH/8-1 downto G_SDRAM_DQ_WIDTH/8);
        end if;
      elsif s_next_state = ST_BURST then
        s_latched_dat_w(G_DATA_WIDTH-1 downto G_DATA_WIDTH-G_SDRAM_DQ_WIDTH) <= (others => '0');
        s_latched_dat_w(G_DATA_WIDTH-G_SDRAM_DQ_WIDTH-1 downto 0) <= s_latched_dat_w(G_DATA_WIDTH-1 downto G_SDRAM_DQ_WIDTH);
        s_latched_sel_n(G_DATA_WIDTH/8-1 downto (G_DATA_WIDTH-G_SDRAM_DQ_WIDTH)/8) <= (others => '1');
        s_latched_sel_n((G_DATA_WIDTH-G_SDRAM_DQ_WIDTH)/8-1 downto 0) <= s_latched_sel_n(G_DATA_WIDTH/8-1 downto G_SDRAM_DQ_WIDTH/8);
      end if;

      -- Select output data & mask signals.
      if s_next_cmd = C_CMD_WR then
        s_dq_out_en <= '1';
        if s_state = ST_SERVICE_REQUEST then
          s_dq_out <= i_dat_w(G_SDRAM_DQ_WIDTH-1 downto 0);
          o_sdram_dqm <= not i_sel((G_SDRAM_DQ_WIDTH/8)-1 downto 0);
        else
          s_dq_out <= s_latched_dat_w(G_SDRAM_DQ_WIDTH-1 downto 0);
          o_sdram_dqm <= s_latched_sel_n((G_SDRAM_DQ_WIDTH/8)-1 downto 0);
        end if;
      elsif s_wr_burst_cnt < C_BURST_LENGTH-1 then
        s_dq_out_en <= '1';
        s_dq_out <= s_latched_dat_w(G_SDRAM_DQ_WIDTH-1 downto 0);
        o_sdram_dqm <= s_latched_sel_n((G_SDRAM_DQ_WIDTH/8)-1 downto 0);
      else
        s_dq_out_en <= '0';
        s_dq_out <= (others => '0');
        o_sdram_dqm <= (others => '0');
      end if;

      -- Was this the final sub-word?
      if (s_next_cmd = C_CMD_WR and C_BURST_LENGTH = 1) or
         (s_wr_burst_cnt = (C_BURST_LENGTH - 2)) then
        v_write_done := '1';
        s_dpl_cycle_cnt <= C_DPL_CYCLES;
      else
        v_write_done := '0';
        if s_dpl_cycle_cnt /= 0 then
          s_dpl_cycle_cnt <= s_dpl_cycle_cnt - 1;
        end if;
      end if;

      -- Update the read burst counter.
      if s_next_cmd = C_CMD_WR then
        s_wr_burst_cnt <= 0;
      elsif s_wr_burst_cnt < (C_BURST_LENGTH-1) then
        s_wr_burst_cnt <= s_wr_burst_cnt + 1;
      end if;

      -- ACK?
      s_ack <= v_read_done or v_write_done;
    end if;
  end process;

end rtl;
