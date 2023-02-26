----------------------------------------------------------------------------------------------------
-- Copyright (c) 2020 Marcus Geelnard
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
-- This is an XRAM implementation for SDRAM memories.
--
-- TODO(m): Add a simple caching mechanism, and transfer more than 32 bits per request.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity xram_sdram is
  generic (
    -- Clock frequency (in Hz)
    CPU_CLK_HZ : integer;

    -- See sdram.vhd for the details of these generics.
    SDRAM_ADDR_WIDTH : integer := 13;
    SDRAM_DATA_WIDTH : integer := 16;
    SDRAM_COL_WIDTH : integer := 9;
    SDRAM_ROW_WIDTH : integer := 13;
    SDRAM_BANK_WIDTH : integer := 2;
    CAS_LATENCY : integer := 2;
    T_DESL : real := 200_000.0;
    T_MRD : real := 12.0;
    T_RC : real := 60.0;
    T_RCD : real := 18.0;
    T_RP : real := 18.0;
    T_DPL : real := 14.0;
    T_REF : real := 64_000_000.0;

    -- FIFO configuration.
    FIFO_DEPTH : integer := 16
  );
  port (
    -- Reset signal.
    i_rst : in std_logic;

    -- Wishbone memory interface (b4 pipelined slave).
    -- See: https://cdn.opencores.org/downloads/wbspec_b4.pdf
    i_wb_clk : in std_logic;
    i_wb_cyc : in std_logic;
    i_wb_stb : in std_logic;
    i_wb_adr : in std_logic_vector(29 downto 0);
    i_wb_dat : in std_logic_vector(31 downto 0);
    i_wb_we : in std_logic;
    i_wb_sel : in std_logic_vector(32/8-1 downto 0);
    o_wb_dat : out std_logic_vector(31 downto 0);
    o_wb_ack : out std_logic;
    o_wb_stall : out std_logic;
    o_wb_err : out std_logic;

    -- SDRAM interface.
    o_sdram_a : out std_logic_vector(SDRAM_ADDR_WIDTH-1 downto 0);
    o_sdram_ba : out std_logic_vector(SDRAM_BANK_WIDTH-1 downto 0);
    io_sdram_dq : inout std_logic_vector(SDRAM_DATA_WIDTH-1 downto 0);
    o_sdram_cke : out std_logic;
    o_sdram_cs_n : out std_logic;
    o_sdram_ras_n : out std_logic;
    o_sdram_cas_n : out std_logic;
    o_sdram_we_n : out std_logic;
    o_sdram_dqm : out std_logic_vector(SDRAM_DATA_WIDTH/8-1 downto 0)
  );
end xram_sdram;

architecture rtl of xram_sdram is
  constant C_ADDR_WIDTH : integer := SDRAM_COL_WIDTH+SDRAM_ROW_WIDTH+SDRAM_BANK_WIDTH-1;
  constant C_DATA_WIDTH : integer := i_wb_dat'length;
  constant C_SEL_WIDTH : integer := C_DATA_WIDTH/8;

  constant C_MEM_OP_WIDTH : integer := C_ADDR_WIDTH + C_DATA_WIDTH + C_SEL_WIDTH + 1;

  -- Input signals.
  signal s_adr : std_logic_vector(C_ADDR_WIDTH-1 downto 0);
  signal s_dat_w : std_logic_vector(C_DATA_WIDTH-1 downto 0);
  signal s_we : std_logic;
  signal s_sel : std_logic_vector(C_SEL_WIDTH-1 downto 0);
  signal s_req : std_logic;

  -- FIFO signals.
  signal s_fifo_wr_en : std_logic;
  signal s_fifo_wr_data : std_logic_vector(C_MEM_OP_WIDTH-1 downto 0);
  signal s_fifo_full : std_logic;
  signal s_fifo_rd_en : std_logic;
  signal s_fifo_rd_data : std_logic_vector(C_MEM_OP_WIDTH-1 downto 0);
  signal s_fifo_empty : std_logic;

  -- Map of FIFO outputs.
  alias a_fifo_rd_adr : std_logic_vector(C_ADDR_WIDTH-1 downto 0) is s_fifo_rd_data(C_ADDR_WIDTH+C_DATA_WIDTH+C_SEL_WIDTH+1-1 downto C_DATA_WIDTH+C_SEL_WIDTH+1);
  alias a_fifo_rd_dat : std_logic_vector(C_DATA_WIDTH-1 downto 0) is s_fifo_rd_data(C_DATA_WIDTH+C_SEL_WIDTH+1-1 downto C_SEL_WIDTH+1);
  alias a_fifo_rd_sel : std_logic_vector(C_SEL_WIDTH-1 downto 0) is s_fifo_rd_data(C_SEL_WIDTH+1-1 downto 1);
  alias a_fifo_rd_we : std_logic is s_fifo_rd_data(0);

  -- Write combiner registers.
  signal s_wc_pending_write : std_logic;
  signal s_wc_adr : std_logic_vector(C_ADDR_WIDTH-1 downto 0);
  signal s_wc_dat : std_logic_vector(C_DATA_WIDTH-1 downto 0);
  signal s_wc_sel : std_logic_vector(C_SEL_WIDTH-1 downto 0);

  -- Write combiner logic
  signal s_wc_adr_match : std_logic;
  signal s_update_wc : std_logic;
  signal s_wc_to_sdram : std_logic;
  signal s_fifo_to_sdram : std_logic;

  -- Result signals.
  signal s_busy : std_logic;
  signal s_ack : std_logic;
  signal s_dat : std_logic_vector(C_DATA_WIDTH-1 downto 0);

  -- Helper function to mix two data words according to a byte select vector.
  function mix_words(a : std_logic_vector(C_DATA_WIDTH-1 downto 0);
                     b : std_logic_vector(C_DATA_WIDTH-1 downto 0);
                     sel : std_logic_vector(C_SEL_WIDTH-1 downto 0))
                     return std_logic_vector is
    variable idx : integer;
    variable result : std_logic_vector(C_DATA_WIDTH-1 downto 0);
  begin
    for k in 0 to C_SEL_WIDTH-1 loop
      idx := k * 8;
      if sel(k) = '0' then
        result(idx+7 downto idx) := a(idx+7 downto idx);
      else
        result(idx+7 downto idx) := b(idx+7 downto idx);
      end if;
    end loop;
    return result;
  end function;

begin
  --------------------------------------------------------------------------------------------------
  -- Memory operation FIFO (queue requests from the Wishbone bus).
  --------------------------------------------------------------------------------------------------

  -- Instantiate the memory operation FIFO.
  fifo_1: entity work.fifo
    generic map (
      G_WIDTH => C_MEM_OP_WIDTH,
      G_DEPTH => FIFO_DEPTH
    )
    port map (
      i_rst => i_rst,
      i_clk => i_wb_clk,
      i_wr_en => s_fifo_wr_en,
      i_wr_data => s_fifo_wr_data,
      o_full => s_fifo_full,
      i_rd_en => s_fifo_rd_en,
      o_rd_data => s_fifo_rd_data,
      o_empty => s_fifo_empty
    );

  -- Write to the FIFO.
  s_fifo_wr_en <= i_wb_cyc and i_wb_stb and not s_fifo_full;
  s_fifo_wr_data <= i_wb_adr(C_ADDR_WIDTH-1 downto 0) &
                    i_wb_dat &
                    i_wb_sel &
                    i_wb_we;

  -- Read from the fifo?
  s_fifo_rd_en <= s_update_wc or s_fifo_to_sdram;


  --------------------------------------------------------------------------------------------------
  -- Write combiner (combine several writes into a single SDRAM request).
  --------------------------------------------------------------------------------------------------

  -- TODO: WE NEED TO SEND ACK:S TO THE WB MASTER WHENEVER WE COLLAPSE A WRITE OPERATION INTO THE
  -- WRITE COMBINER. AND THE ACK:S NEED TO COME IN THE RIGHT ORDER (CONSIDER READ ACK:S).

  -- +-------+---------+-------+-------+-------||--------+-------+---------+------+
  -- | ADR   | WC      | FIFO  | FIFO  | SDRAM || Update | WC to | FIFO to | Read |
  -- | match | pending | empty | we    | busy  || WC     | SDRAM | SDRAM   | FIFO |
  -- +-------+---------+-------+-------+-------||--------+-------+---------+------+
  -- |   -   |    0    |   0   |   0   |   0   ||    0   |   0   |    1    |   1  |
  -- |   -   |    0    |   0   |   0   |   1   ||    0   |   0   |    0    |   0  |
  -- |   -   |    0    |   0   |   1   |   0   ||    0   |   0   |    1    |   1  |
  -- |   -   |    0    |   0   |   1   |   1   ||    1   |   0   |    0    |   1  |
  -- |   -   |    0    |   1   |   -   |   0   ||    0   |   0   |    0    |   0  |
  -- |   -   |    0    |   1   |   -   |   1   ||    0   |   0   |    0    |   0  |
  -- |   -   |    0    |   1   |   -   |   0   ||    0   |   0   |    0    |   0  |
  -- |   -   |    0    |   1   |   -   |   1   ||    0   |   0   |    0    |   0  |
  -- |   0   |    1    |   0   |   0   |   0   ||    0   |   1   |    0    |   0  |
  -- |   0   |    1    |   0   |   0   |   1   ||    0   |   0   |    0    |   0  |
  -- |   0   |    1    |   0   |   1   |   0   ||    0   |   1   |    0    |   0  |
  -- |   0   |    1    |   0   |   1   |   1   ||    0   |   0   |    0    |   0  |
  -- |   -   |    1    |   1   |   -   |   0   ||    0   |   1   |    0    |   0  |
  -- |   -   |    1    |   1   |   -   |   1   ||    0   |   0   |    0    |   0  |
  -- |   -   |    1    |   1   |   -   |   0   ||    0   |   1   |    0    |   0  |
  -- |   -   |    1    |   1   |   -   |   1   ||    0   |   0   |    0    |   0  |
  -- +-------+---------+-------+-------+-------||--------+-------+---------+------+
  -- |   -   |    0    |   0   |   0   |   0   ||    0   |   0   |    1    |   1  |
  -- |   -   |    0    |   0   |   0   |   1   ||    0   |   0   |    0    |   0  |
  -- |   -   |    0    |   0   |   1   |   0   ||    0   |   0   |    1    |   1  |
  -- |   -   |    0    |   0   |   1   |   1   ||    1   |   0   |    0    |   1  |
  -- |   -   |    0    |   1   |   -   |   0   ||    0   |   0   |    0    |   0  |
  -- |   -   |    0    |   1   |   -   |   1   ||    0   |   0   |    0    |   0  |
  -- |   -   |    0    |   1   |   -   |   0   ||    0   |   0   |    0    |   0  |
  -- |   -   |    0    |   1   |   -   |   1   ||    0   |   0   |    0    |   0  |
  -- |   1   |    1    |   0   |   0   |   0   ||    0   |   1   |    0    |   0  |
  -- |   1   |    1    |   0   |   0   |   1   ||    0   |   0   |    0    |   0  |
  -- |   1   |    1    |   0   |   1   |   0   ||    1   |   0   |    0    |   1  |
  -- |   1   |    1    |   0   |   1   |   1   ||    1   |   0   |    0    |   1  |
  -- |   -   |    1    |   1   |   -   |   0   ||    0   |   1   |    0    |   0  |
  -- |   -   |    1    |   1   |   -   |   1   ||    0   |   0   |    0    |   0  |
  -- |   -   |    1    |   1   |   -   |   0   ||    0   |   1   |    0    |   0  |
  -- |   -   |    1    |   1   |   -   |   1   ||    0   |   0   |    0    |   0  |
  -- +-------+---------+-------+-------+-------||--------+-------+---------+------+

  -- Does the next memory operation from the FIFO match the address of the write combiner?
  s_wc_adr_match <= '1' when s_wc_adr = a_fifo_rd_adr else '0';

  -- Should we update the write combiner?
  s_update_wc <= (not s_fifo_empty) and s_fifo_rd_we and
                   (((not s_wc_pending_write) and s_busy) or
                    (s_wc_pending_write and s_wc_adr_match));

  -- Should we send the contents of the write combiner to the SDRAM?
  s_wc_to_sdram <= (not s_busy) and s_wc_pending_write and
                   not (s_wc_adr_match and (not s_fifo_empty) and s_fifo_rd_we);

  -- Should we send the next FIFO operation to the SDRAM?
  s_fifo_to_sdram <= (not s_busy) and (not s_fifo_empty) and (not s_wc_pending_write);

  process (i_rst, i_clk) is
  begin
    if i_rst = '1' then
      s_wc_pending_write <= '0';
      s_wc_adr <= (others => '0');
      s_wc_dat <= (others => '0');
      s_wc_sel <= (others => '0');
    elsif rising_edge(i_clk) then
      if s_update_wc = '1' then
        if s_wc_adr_match = '1' and s_wc_pending_write = '1' then
          -- Combine DAT according to SEL.
          s_wc_dat <= mix_words(s_wc_dat, a_fifo_rd_dat, a_fifo_rd_sel);
          s_wc_sel <= s_wc_sel or a_fifo_rd_sel;
        else
          -- Over-write DAT.
          s_wc_dat <= a_fifo_rd_dat;
          s_wc_sel <= a_fifo_rd_sel;
        end if;

        s_wc_adr <= a_fifo_rd_adr;
        s_wc_pending_write <= '1';
      elsif s_wc_to_sdram = '1' then
        s_wc_pending_write <= '0';
      end if;
    end if;
  end if;


  --------------------------------------------------------------------------------------------------
  -- SDRAM controller.
  --------------------------------------------------------------------------------------------------

  -- Send request from the FIFO or the write combiner to the SDRAM controller.
  s_req   <= s_fifo_to_sdram or s_wc_to_sdram;
  s_adr   <= s_wc_adr when s_wc_to_sdram = '1' else a_fifo_rd_adr;
  s_dat_w <= s_wc_dat when s_wc_to_sdram = '1' else a_fifo_rd_dat;
  s_sel   <= s_wc_sel when s_wc_to_sdram = '1' else a_fifo_rd_sel;
  s_we    <= '1'      when s_wc_to_sdram = '1' else a_fifo_rd_we;

  -- Instantiate the SDRAM controller.
  sdram_controller_1: entity work.sdram
    generic map (
      G_CLK_FREQ_HZ => CPU_CLK_HZ,
      G_ADDR_WIDTH => C_ADDR_WIDTH,
      G_DATA_WIDTH => C_DATA_WIDTH,
      G_SDRAM_A_WIDTH => SDRAM_ADDR_WIDTH,
      G_SDRAM_DQ_WIDTH => SDRAM_DATA_WIDTH,
      G_SDRAM_BA_WIDTH => SDRAM_BANK_WIDTH,
      G_SDRAM_COL_WIDTH => SDRAM_COL_WIDTH,
      G_SDRAM_ROW_WIDTH => SDRAM_ROW_WIDTH,
      G_CAS_LATENCY => CAS_LATENCY,
      G_T_DESL => T_DESL,
      G_T_MRD => T_MRD,
      G_T_RC => T_RC,
      G_T_RCD => T_RCD,
      G_T_RP => T_RP,
      G_T_DPL => T_DPL,
      G_T_REF => T_REF
    )
    port map (
      i_rst => i_rst,
      i_clk => i_wb_clk,

      -- CPU/Wishbone interface.
      i_adr => s_adr,
      i_dat_w => s_dat_w,
      i_we => s_we,
      i_sel => s_sel,
      i_req => s_req,
      o_busy => s_busy,
      o_ack => s_ack,
      o_dat => s_dat,

      -- External SDRAM interface.
      o_sdram_a => o_sdram_a,
      o_sdram_ba => o_sdram_ba,
      io_sdram_dq => io_sdram_dq,
      o_sdram_cke => o_sdram_cke,
      o_sdram_cs_n => o_sdram_cs_n,
      o_sdram_ras_n => o_sdram_ras_n,
      o_sdram_cas_n => o_sdram_cas_n,
      o_sdram_we_n => o_sdram_we_n,
      o_sdram_dqm => o_sdram_dqm
    );

  -- Wishbone outputs.
  o_wb_stall <= s_fifo_full;
  o_wb_ack <= s_ack;
  o_wb_dat <= s_dat;
  o_wb_err <= '0';
end rtl;

