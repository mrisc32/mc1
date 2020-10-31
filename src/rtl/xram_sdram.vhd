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
    SDRAM_ADDR_WIDTH : natural := 13;
    SDRAM_DATA_WIDTH : natural := 16;
    SDRAM_COL_WIDTH : natural := 9;
    SDRAM_ROW_WIDTH : natural := 13;
    SDRAM_BANK_WIDTH : natural := 2;
    CAS_LATENCY : natural := 2;
    T_DESL : real := 200000.0;
    T_MRD : real := 12.0;
    T_RC : real := 60.0;
    T_RCD : real := 18.0;
    T_RP : real := 18.0;
    T_WR : real := 12.0;
    T_REFI : real := 7800.0
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
  -- Address bits for 32-bit words (i.e. log2(bytesize)-2)
  constant C_ADDR_WIDTH : natural := SDRAM_COL_WIDTH+SDRAM_ROW_WIDTH+SDRAM_BANK_WIDTH-1;

  signal s_addr : unsigned(C_ADDR_WIDTH-1 downto 0);
  signal s_req : std_logic;
  signal s_ready : std_logic;
  signal s_ack : std_logic;
  signal s_valid : std_logic;

  signal s_sdram_a : unsigned(SDRAM_ADDR_WIDTH-1 downto 0);
  signal s_sdram_ba : unsigned(SDRAM_BANK_WIDTH-1 downto 0);

  signal s_req_from_wb : std_logic;
  signal s_start_req : std_logic;
  signal s_wating_for_ack : std_logic;
  signal s_waiting_for_read_response : std_logic;
  signal s_waiting_for_write_response : std_logic;
begin
  -- Convert the Wishbone address to an address for the SDRAM controller.
  s_addr <= unsigned(i_wb_adr(C_ADDR_WIDTH-1 downto 0));

  -- Should & can we start a new request?
  s_req_from_wb <= i_wb_cyc and i_wb_stb;
  s_start_req <= s_req_from_wb and s_ready;

  -- Keep the SDRAM controller REQ signal high until we've got the final ACK.
  -- TODO(m): Correct/necessary?
  s_req <= s_start_req or s_wating_for_ack;

  -- Wishbone outputs.
  o_wb_ack <= (s_waiting_for_read_response and s_valid) or
              (s_waiting_for_write_response and s_ack);
  o_wb_stall <= not s_ready;
  o_wb_err <= '0';

  -- Convert some SDRAM outputs to SLV.
  o_sdram_a <= std_logic_vector(s_sdram_a);
  o_sdram_ba <= std_logic_vector(s_sdram_ba);

  -- Keep track of ongoing requests.
  process (i_rst, i_wb_clk)
  begin
    if i_rst = '1' then
      s_wating_for_ack <= '0';
      s_waiting_for_read_response <= '0';
      s_waiting_for_write_response <= '0';
    elsif rising_edge(i_wb_clk) then
      if s_start_req = '1' then
        s_wating_for_ack <= '1';
        s_waiting_for_read_response <= not i_wb_we;
        s_waiting_for_write_response <= i_wb_we;
      else
        if s_ack = '1' then
          s_wating_for_ack <= '0';
          s_waiting_for_write_response <= '0';
        end if;
        if s_valid = '1' then
          s_waiting_for_read_response <= '0';
        end if;
      end if;
    end if;
  end process;

  -- Instantiate the SDRAM controller.
  sdram_controller_1: entity work.sdram
    generic map (
      CLK_FREQ => real(CPU_CLK_HZ)*0.000001,
      ADDR_WIDTH => C_ADDR_WIDTH,
      DATA_WIDTH => 32,
      SDRAM_ADDR_WIDTH => SDRAM_ADDR_WIDTH,
      SDRAM_DATA_WIDTH => SDRAM_DATA_WIDTH,
      SDRAM_COL_WIDTH => SDRAM_COL_WIDTH,
      SDRAM_ROW_WIDTH => SDRAM_ROW_WIDTH,
      SDRAM_BANK_WIDTH => SDRAM_BANK_WIDTH,
      CAS_LATENCY => CAS_LATENCY,
      BURST_LENGTH => 2,
      T_DESL => T_DESL,
      T_MRD => T_MRD,
      T_RC => T_RC,
      T_RCD => T_RCD,
      T_RP => T_RP,
      T_WR => T_WR,
      T_REFI => T_REFI
    )
    port map (
      reset  => i_rst,
      clk => i_wb_clk,

      -- CPU/Wishbone interface.
      addr => s_addr,
      data => i_wb_dat,
      we => i_wb_we,
      sel => i_wb_sel,
      req => s_req,
      ready => s_ready,
      ack => s_ack,
      valid => s_valid,
      q => o_wb_dat,

      -- External SDRAM interface.
      sdram_a => s_sdram_a,
      sdram_ba => s_sdram_ba,
      sdram_dq => io_sdram_dq,
      sdram_cke => o_sdram_cke,
      sdram_cs_n => o_sdram_cs_n,
      sdram_ras_n => o_sdram_ras_n,
      sdram_cas_n => o_sdram_cas_n,
      sdram_we_n => o_sdram_we_n,
      sdram_dqm => o_sdram_dqm
    );

end architecture rtl;
