--   __   __     __  __     __         __
--  /\ "-.\ \   /\ \/\ \   /\ \       /\ \
--  \ \ \-.  \  \ \ \_\ \  \ \ \____  \ \ \____
--   \ \_\\"\_\  \ \_____\  \ \_____\  \ \_____\
--    \/_/ \/_/   \/_____/   \/_____/   \/_____/
--   ______     ______       __     ______     ______     ______
--  /\  __ \   /\  == \     /\ \   /\  ___\   /\  ___\   /\__  _\
--  \ \ \/\ \  \ \  __<    _\_\ \  \ \  __\   \ \ \____  \/_/\ \/
--   \ \_____\  \ \_____\ /\_____\  \ \_____\  \ \_____\    \ \_\
--    \/_____/   \/_____/ \/_____/   \/_____/   \/_____/     \/_/
--
-- https://joshbassett.info
-- https://twitter.com/nullobject
-- https://github.com/nullobject
--
-- Copyright (c) 2020 Josh Bassett
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

-- This SDRAM controller provides a symmetric 32-bit synchronous read/write
-- interface for a 16Mx16-bit SDRAM chip (e.g. AS4C16M16SA-6TCN, IS42S16400F,
-- etc.).
entity sdram is
  generic (
    -- clock frequency (in MHz)
    --
    -- This value must be provided, as it is used to calculate the number of
    -- clock cycles required for the other timing values.
    CLK_FREQ : real;

    -- 32-bit controller interface
    ADDR_WIDTH : natural := 23;
    DATA_WIDTH : natural := 32;

    -- SDRAM interface
    SDRAM_ADDR_WIDTH : natural := 13;
    SDRAM_DATA_WIDTH : natural := 16;
    SDRAM_COL_WIDTH  : natural := 9;
    SDRAM_ROW_WIDTH  : natural := 13;
    SDRAM_BANK_WIDTH : natural := 2;

    -- The delay in clock cycles, between the start of a read command and the
    -- availability of the output data.
    CAS_LATENCY : natural := 2; -- 2=below 133MHz, 3=above 133MHz

    -- The number of 16-bit words to be bursted during a read/write.
    BURST_LENGTH : natural := 2;

    -- timing values (in nanoseconds)
    --
    -- These values can be adjusted to match the exact timing of your SDRAM
    -- chip (refer to the datasheet).
    T_DESL : real := 200000.0; -- startup delay
    T_MRD  : real :=     12.0; -- mode register cycle time
    T_RC   : real :=     60.0; -- row cycle time
    T_RCD  : real :=     18.0; -- RAS to CAS delay
    T_RP   : real :=     18.0; -- precharge to activate delay
    T_WR   : real :=     12.0; -- write recovery time
    T_REFI : real :=   7800.0  -- average refresh interval
  );
  port (
    -- reset
    reset : in std_logic := '0';

    -- clock
    clk : in std_logic;

    -- address bus
    addr : in unsigned(ADDR_WIDTH-1 downto 0);

    -- input data bus
    data : in std_logic_vector(DATA_WIDTH-1 downto 0);

    -- When the write enable signal is asserted, a write operation will be performed.
    we : in std_logic;

    -- Byte select for write operations ('1' = enable byte)
    sel : in std_logic_vector(DATA_WIDTH/8-1 downto 0);

    -- When the request signal is asserted, an operation will be performed.
    req : in std_logic;

    -- The ready signal is asserted when the controller is ready to accept a
    -- new request.
    ready : out std_logic;

    -- The acknowledge signal is asserted by the SDRAM controller when
    -- a request has been accepted.
    ack : out std_logic;

    -- The valid signal is asserted when there is a valid word on the output
    -- data bus.
    valid : out std_logic;

    -- output data bus
    q : out std_logic_vector(DATA_WIDTH-1 downto 0);

    -- SDRAM interface (e.g. AS4C16M16SA-6TCN, IS42S16400F, etc.)
    sdram_a     : out unsigned(SDRAM_ADDR_WIDTH-1 downto 0);
    sdram_ba    : out unsigned(SDRAM_BANK_WIDTH-1 downto 0);
    sdram_dq    : inout std_logic_vector(SDRAM_DATA_WIDTH-1 downto 0);
    sdram_cke   : out std_logic;
    sdram_cs_n  : out std_logic;
    sdram_ras_n : out std_logic;
    sdram_cas_n : out std_logic;
    sdram_we_n  : out std_logic;
    sdram_dqm   : out std_logic_vector(SDRAM_DATA_WIDTH/8-1 downto 0)
  );

  -- Use fast I/O flip-flops for the SDRAM output signals.
  attribute useioff : boolean;
  attribute useioff of sdram_a : signal is true;
  attribute useioff of sdram_ba : signal is true;
  attribute useioff of sdram_cke : signal is true;
  attribute useioff of sdram_cs_n : signal is true;
  attribute useioff of sdram_ras_n : signal is true;
  attribute useioff of sdram_cas_n : signal is true;
  attribute useioff of sdram_we_n : signal is true;
  attribute useioff of sdram_dqm : signal is true;
end sdram;

architecture arch of sdram is
  function ilog2(n : natural) return natural is
  begin
    return natural(ceil(log2(real(n))));
  end ilog2;

  -- Convert a ROW address to a signal suitable for sdram_a.
  function row2addr(x : unsigned) return unsigned is
  begin
    return resize(x, SDRAM_ADDR_WIDTH);
  end row2addr;

  -- Convert a COL address to a signal suitable for sdram_a.
  function col2addr(x : unsigned) return unsigned is
    variable a : unsigned(SDRAM_ADDR_WIDTH-2 downto 0);
  begin
    a := resize(x, SDRAM_ADDR_WIDTH-1);
    -- A10 = '1' -> auto precharge
    return a(SDRAM_ADDR_WIDTH-2 downto 10) & "1" & a(9 downto 0);
  end col2addr;

  -- Adjust the incoming address to the SDRAM address space (e.g.
  -- from 32-bit word addressing to 16-bit word addressing).
  function adjust_addr(x : unsigned) return unsigned is
    constant C_SHIFT : natural := ilog2(DATA_WIDTH / SDRAM_DATA_WIDTH);
  begin
    return x & to_unsigned(0, C_SHIFT);
  end adjust_addr;

  subtype command_t is std_logic_vector(3 downto 0);

  -- commands
  constant CMD_DESELECT     : command_t := "1---";
  constant CMD_LOAD_MODE    : command_t := "0000";
  constant CMD_AUTO_REFRESH : command_t := "0001";
  constant CMD_PRECHARGE    : command_t := "0010";
  constant CMD_ACTIVE       : command_t := "0011";
  constant CMD_WRITE        : command_t := "0100";
  constant CMD_READ         : command_t := "0101";
  constant CMD_STOP         : command_t := "0110";
  constant CMD_NOP          : command_t := "0111";

  -- the ordering of the accesses within a burst
  constant BURST_TYPE : std_logic := '0'; -- 0=sequential, 1=interleaved

  -- the write burst mode enables bursting for write operations
  constant WRITE_BURST_MODE : std_logic := '0'; -- 0=burst, 1=single

  -- the value written to the address bus during initialization
  constant INIT_CMD : unsigned(SDRAM_ADDR_WIDTH-1 downto 0) := (
    to_unsigned(0, SDRAM_ADDR_WIDTH-11) &
    "10000000000"
  );

  -- the value written to the mode register to configure the memory
  constant MODE_REG : unsigned(SDRAM_ADDR_WIDTH-1 downto 0) := (
    to_unsigned(0, SDRAM_ADDR_WIDTH-10) &
    WRITE_BURST_MODE &
    "00" &
    to_unsigned(CAS_LATENCY, 3) &
    BURST_TYPE &
    to_unsigned(ilog2(BURST_LENGTH), 3)
  );

  -- calculate the clock period (in nanoseconds)
  constant CLK_PERIOD : real := 1.0/CLK_FREQ*1000.0;

  -- the number of clock cycles to wait before initialising the device
  constant INIT_WAIT : natural := natural(ceil(T_DESL/CLK_PERIOD));

  -- the number of clock cycles to wait while a LOAD MODE command is being
  -- executed
  constant LOAD_MODE_WAIT : natural := natural(ceil(T_MRD/CLK_PERIOD));

  -- the number of clock cycles to wait while an ACTIVE command is being
  -- executed
  constant ACTIVE_WAIT : natural := natural(ceil(T_RCD/CLK_PERIOD));

  -- the number of clock cycles to wait while a REFRESH command is being
  -- executed
  constant REFRESH_WAIT : natural := natural(ceil(T_RC/CLK_PERIOD));

  -- the number of clock cycles to wait while a PRECHARGE command is being
  -- executed
  constant PRECHARGE_WAIT : natural := natural(ceil(T_RP/CLK_PERIOD));

  -- the number of clock cycles to wait while a READ command is being executed
  constant READ_WAIT : natural := CAS_LATENCY+BURST_LENGTH;

  -- the number of clock cycles to wait while a WRITE command is being executed
  constant WRITE_WAIT : natural := BURST_LENGTH+natural(ceil((T_WR+T_RP)/CLK_PERIOD));

  -- the number of clock cycles before the memory controller needs to refresh
  -- the SDRAM
  constant REFRESH_INTERVAL : natural := natural(floor(T_REFI/CLK_PERIOD))-10;

  type state_t is (INIT, MODE, IDLE, ACTIVE, READ, WRITE, REFRESH);

  -- state signals
  signal state, next_state : state_t;

  -- command signals
  signal cmd, next_cmd : command_t := CMD_NOP;

  -- control signals
  signal start          : std_logic;
  signal load_mode_done : std_logic;
  signal active_done    : std_logic;
  signal refresh_done   : std_logic;
  signal read_done      : std_logic;
  signal write_done     : std_logic;
  signal should_refresh : std_logic;

  -- counters
  constant MAX_WAIT_COUNT    : natural := INIT_WAIT+PRECHARGE_WAIT+REFRESH_WAIT+REFRESH_WAIT+1;
  constant MAX_REFRESH_COUNT : natural := REFRESH_INTERVAL;
  signal wait_counter    : natural range 0 to MAX_WAIT_COUNT;
  signal refresh_counter : natural range 0 to MAX_REFRESH_COUNT;

  -- registers
  signal addr_reg  : unsigned(ADDR_WIDTH-1 downto 0);
  signal data_reg  : std_logic_vector(DATA_WIDTH-1 downto 0);
  signal we_reg    : std_logic;
  signal sel_n_reg : std_logic_vector(DATA_WIDTH/8-1 downto 0);
  signal q_reg     : std_logic_vector(DATA_WIDTH-1 downto 0);

  -- DQ in/out signals
  signal dq_in : std_logic_vector(SDRAM_DATA_WIDTH-1 downto 0);
  signal dq_out : std_logic_vector(SDRAM_DATA_WIDTH-1 downto 0);
  signal dq_out_en : std_logic;

  -- aliases to decode the address
  signal addr_current : unsigned(SDRAM_COL_WIDTH+SDRAM_ROW_WIDTH+SDRAM_BANK_WIDTH-1 downto 0);
  alias col  : unsigned(SDRAM_COL_WIDTH-1 downto 0) is addr_current(SDRAM_COL_WIDTH-1 downto 0);
  alias row  : unsigned(SDRAM_ROW_WIDTH-1 downto 0) is addr_current(SDRAM_COL_WIDTH+SDRAM_ROW_WIDTH-1 downto SDRAM_COL_WIDTH);
  alias bank : unsigned(SDRAM_BANK_WIDTH-1 downto 0) is addr_current(SDRAM_COL_WIDTH+SDRAM_ROW_WIDTH+SDRAM_BANK_WIDTH-1 downto SDRAM_COL_WIDTH+SDRAM_ROW_WIDTH);

  -- Use fast I/O flip-flops for the SDRAM data in/out signals.
  attribute useioff of dq_in : signal is true;
  attribute useioff of dq_out : signal is true;
begin
  -- state machine
  fsm : process (state, wait_counter, req, we_reg, sel_n_reg, load_mode_done, active_done, refresh_done, read_done, write_done, should_refresh)
  begin
    next_state <= state;

    -- default to a NOP command
    next_cmd <= CMD_NOP;

    case state is
      -- execute the initialisation sequence
      when INIT =>
        if wait_counter = 0 then
          next_cmd <= CMD_DESELECT;
        elsif wait_counter = INIT_WAIT-1 then
          next_cmd <= CMD_PRECHARGE;
        elsif wait_counter = INIT_WAIT+PRECHARGE_WAIT-1 then
          next_cmd <= CMD_AUTO_REFRESH;
        elsif wait_counter = INIT_WAIT+PRECHARGE_WAIT+REFRESH_WAIT-1 then
          next_cmd <= CMD_AUTO_REFRESH;
        elsif wait_counter = INIT_WAIT+PRECHARGE_WAIT+REFRESH_WAIT+REFRESH_WAIT-1 then
          next_state <= MODE;
          next_cmd   <= CMD_LOAD_MODE;
        end if;

      -- load the mode register
      when MODE =>
        if load_mode_done = '1' then
          next_state <= IDLE;
        end if;

      -- wait for a read/write request
      when IDLE =>
        if should_refresh = '1' then
          next_state <= REFRESH;
          next_cmd   <= CMD_AUTO_REFRESH;
        elsif req = '1' then
          next_state <= ACTIVE;
          next_cmd   <= CMD_ACTIVE;
        end if;

      -- activate the row
      when ACTIVE =>
        if active_done = '1' then
          if we_reg = '1' then
            next_state <= WRITE;
            next_cmd   <= CMD_WRITE;
          else
            next_state <= READ;
            next_cmd   <= CMD_READ;
          end if;
        end if;

      -- execute a read command
      when READ =>
        if read_done = '1' then
          if should_refresh = '1' then
            next_state <= REFRESH;
            next_cmd   <= CMD_AUTO_REFRESH;
          elsif req = '1' then
            next_state <= ACTIVE;
            next_cmd   <= CMD_ACTIVE;
          else
            next_state <= IDLE;
          end if;
        end if;

      -- execute a write command
      when WRITE =>
        if write_done = '1' then
          if should_refresh = '1' then
            next_state <= REFRESH;
            next_cmd   <= CMD_AUTO_REFRESH;
          elsif req = '1' then
            next_state <= ACTIVE;
            next_cmd   <= CMD_ACTIVE;
          else
            next_state <= IDLE;
          end if;
        end if;

      -- execute an auto refresh
      when REFRESH =>
        if refresh_done = '1' then
          if req = '1' then
            next_state <= ACTIVE;
            next_cmd   <= CMD_ACTIVE;
          else
            next_state <= IDLE;
          end if;
        end if;
    end case;
  end process;

  -- latch the next state
  latch_next_state : process (clk, reset)
  begin
    if reset = '1' then
      state <= INIT;
      cmd   <= CMD_NOP;
    elsif rising_edge(clk) then
      state <= next_state;
      cmd   <= next_cmd;
    end if;
  end process;

  -- the wait counter is used to hold the current state for a number of clock
  -- cycles
  update_wait_counter : process (clk, reset)
  begin
    if reset = '1' then
      wait_counter <= 0;
    elsif rising_edge(clk) then
      if state /= next_state then -- state changing
        wait_counter <= 0;
      elsif state = IDLE then    -- counter would overflow when IDLE
        wait_counter <= 0;
      else
        wait_counter <= wait_counter + 1;
      end if;
    end if;
  end process;

  -- the refresh counter is used to periodically trigger a refresh operation
  update_refresh_counter : process (clk, reset)
  begin
    if reset = '1' then
      refresh_counter <= 0;
      should_refresh <= '0';
    elsif rising_edge(clk) then
      -- Update the refresh counter.
      if state = REFRESH and wait_counter = 0 then
        refresh_counter <= 0;
      elsif refresh_counter /= MAX_REFRESH_COUNT then
        refresh_counter <= refresh_counter + 1;
      end if;

      -- Time for a refresh?
      if refresh_counter = REFRESH_INTERVAL-2 then
        should_refresh <= '1';
      elsif state /= REFRESH and next_state = REFRESH then
        should_refresh <= '0';
      end if;
    end if;
  end process;

  -- latch the request
  latch_request : process (reset, clk)
  begin
    if reset = '1' then
      addr_reg <= (others => '0');
      data_reg <= (others => '0');
      we_reg <= '0';
      sel_n_reg <= (others => '0');
    elsif rising_edge(clk) then
      if start = '1' then
        addr_reg  <= addr;
        data_reg  <= data;
        we_reg    <= we;
        sel_n_reg <= not sel;
      end if;
    end if;
  end process;

  -- set wait signals
  load_mode_done <= '1' when wait_counter = LOAD_MODE_WAIT-1 else '0';
  active_done    <= '1' when wait_counter = ACTIVE_WAIT-1    else '0';
  refresh_done   <= '1' when wait_counter = REFRESH_WAIT-1   else '0';
  read_done      <= '1' when wait_counter = READ_WAIT-1      else '0';
  write_done     <= '1' when wait_counter = WRITE_WAIT-1     else '0';

  -- a new request is only allowed at the end of the IDLE, READ, WRITE, and
  -- REFRESH states, as long as a refresh is not pending.
  start <= (not should_refresh) when
               (state = IDLE) or
               (state = READ and read_done = '1') or
               (state = WRITE and write_done = '1') or
               (state = REFRESH and refresh_done = '1') else '0';

  -- assert the ready signal when we're ready to accept a new request
  ready <= start;

  -- assert the acknowledge signal at the beginning of the ACTIVE state
  process (reset, clk)
  begin
    if reset = '1' then
      ack <= '0';
    elsif rising_edge(clk) then
      if next_state = ACTIVE and next_state /= state then
        ack <= '1';
      else
        ack <= '0';
      end if;
    end if;
  end process;

  -- set output data
  q <= q_reg;

  -- assert the clock enable signal once we have entered the INIT state
  process (reset, clk)
  begin
    if reset = '1' then
      sdram_cke <= '0';
    elsif rising_edge(clk) then
      if state = INIT then
        sdram_cke <= '1';
      end if;
    end if;
  end process;

  -- set SDRAM control signals
  (sdram_cs_n, sdram_ras_n, sdram_cas_n, sdram_we_n) <= cmd;

  -- set SDRAM bank and address
  addr_current <= adjust_addr(addr) when start = '1' else adjust_addr(addr_reg);
  process (reset, clk)
  begin
    if reset = '1' then
      sdram_ba <= (others => '0');
      sdram_a <= (others => '0');
    elsif rising_edge(clk) then
      case next_state is
        when ACTIVE | READ | WRITE =>
          sdram_ba <= bank;
        when others =>
          sdram_ba <= (others => '0');
      end case;

      case next_state is
        when INIT =>
          sdram_a <= INIT_CMD;
        when MODE =>
          sdram_a <= MODE_REG;
        when ACTIVE =>
          sdram_a <= row2addr(row);
        when READ | WRITE =>
          sdram_a <= col2addr(col);
        when others =>
          sdram_a <= (others => '0');
      end case;
    end if;
  end process;

  -- read the next sub-word as it's bursted from the SDRAM
  process (reset, clk)
    -- Add one extra cycle delay due to SDRAM clock phase diff.
    constant C_START_CNT : integer := -1;
    variable v_burst_cnt : integer range C_START_CNT to BURST_LENGTH := BURST_LENGTH;
  begin
    if reset = '1' then
      q_reg <= (others => '0');
      valid <= '0';
    elsif rising_edge(clk) then
      if state = READ and wait_counter = CAS_LATENCY then
        v_burst_cnt := C_START_CNT;
      elsif v_burst_cnt < BURST_LENGTH then
        v_burst_cnt := v_burst_cnt + 1;
      end if;

      if v_burst_cnt >= 0 and v_burst_cnt < BURST_LENGTH then
        q_reg(SDRAM_DATA_WIDTH*(v_burst_cnt+1)-1 downto SDRAM_DATA_WIDTH*v_burst_cnt) <= dq_in;
      end if;

      -- Was this the final sub-word?
      if v_burst_cnt = (BURST_LENGTH - 1) then
        valid <= '1';
      else
        valid <= '0';
      end if;
    end if;
  end process;

  -- write the next sub-word from the write buffer
  process (reset, clk)
    variable v_burst_cnt : natural range 0 to BURST_LENGTH := BURST_LENGTH;
  begin
    if reset = '1' then
      dq_out_en <= '0';
      dq_out <= (others => '0');
      sdram_dqm <= (others => '0');
    elsif rising_edge(clk) then
      if next_state = WRITE and next_state /= state then
        -- Start a new write burst.
        v_burst_cnt := 0;
      elsif v_burst_cnt < BURST_LENGTH then
        v_burst_cnt := v_burst_cnt + 1;
      end if;

      if v_burst_cnt < BURST_LENGTH then
        dq_out_en <= '1';
        dq_out <= data_reg(SDRAM_DATA_WIDTH*(v_burst_cnt+1)-1 downto SDRAM_DATA_WIDTH*v_burst_cnt);
        sdram_dqm <= sel_n_reg((SDRAM_DATA_WIDTH/8)*(v_burst_cnt+1)-1 downto (SDRAM_DATA_WIDTH/8)*v_burst_cnt);
      else
        dq_out_en <= '0';
        dq_out <= (others => '0');
        sdram_dqm <= (others => '0');
      end if;
    end if;
  end process;

  ---------------------------------------------------------------------------
  -- SDRAM data interface - Since the SDRAM_DQ signal is an inout signal, we
  -- need to use constructs that map well to FPGA I/O buffers.
  ---------------------------------------------------------------------------

  -- Sample the input DQ signal.
  process (reset, clk)
  begin
    if reset = '1' then
      dq_in <= (others => '0');
    elsif rising_edge(clk) then
      dq_in <= sdram_dq;
    end if;
  end process;

  -- This should infer an IOBUF.
  sdram_dq <= dq_out when dq_out_en = '1' else (others => 'Z');

end architecture arch;
