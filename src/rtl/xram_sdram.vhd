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
    T_REF : real := 64_000_000.0
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

  -- Input signals.
  signal s_adr : std_logic_vector(C_ADDR_WIDTH-1 downto 0);
  signal s_dat_w : std_logic_vector(C_DATA_WIDTH-1 downto 0);
  signal s_we : std_logic;
  signal s_sel : std_logic_vector(C_DATA_WIDTH/8-1 downto 0);
  signal s_req : std_logic;

  -- Result signals.
  signal s_busy : std_logic;
  signal s_ack : std_logic;
  signal s_dat : std_logic_vector(C_DATA_WIDTH-1 downto 0);
begin
  -- Wishbone adaptations.
  s_adr <= i_wb_adr(C_ADDR_WIDTH-1 downto 0);
  s_dat_w <= i_wb_dat;
  s_we <= i_wb_we;
  s_sel <= i_wb_sel;
  s_req <= i_wb_cyc and i_wb_stb;
  o_wb_stall <= s_busy;
  o_wb_ack <= s_ack;
  o_wb_dat <= s_dat;
  o_wb_err <= '0';

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
      i_rst  => i_rst,
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

end architecture rtl;
