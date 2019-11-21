----------------------------------------------------------------------------------------------------
-- Copyright (c) 2019 Marcus Geelnard
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
-- This is the top level entity for DE0-CV.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity toplevel is
  port(
    -- RESET_N "3.3-V LVTTL"
    RESET_N : in std_logic;

    -- Clocks "3.3-V LVTTL"
    CLOCK_50 : in std_logic;
    CLOCK2_50 : in std_logic;
    CLOCK3_50 : in std_logic;
    CLOCK4_50 : in std_logic;

    -- DRAM  "3.3-V LVTTL"
    DRAM_ADDR : out std_logic_vector(12 downto 0);
    DRAM_BA : out std_logic_vector(1 downto 0);
    DRAM_CAS_N : out std_logic;
    DRAM_CKE : out std_logic;
    DRAM_CLK : out std_logic;
    DRAM_CS_N : out std_logic;
    DRAM_DQ : inout std_logic_vector(15 downto 0);
    DRAM_LDQM : out std_logic;
    DRAM_RAS_N : out std_logic;
    DRAM_UDQM : out std_logic;
    DRAM_WE_N : out std_logic;

    -- GPIO "3.3-V LVTTL"
    GPIO_0 : inout std_logic_vector(35 downto 0);
    GPIO_1 : inout std_logic_vector(35 downto 0);

    -- HEX0-5  "3.3-V LVTTL"
    HEX0 : out std_logic_vector(6 downto 0);
    HEX1 : out std_logic_vector(6 downto 0);
    HEX2 : out std_logic_vector(6 downto 0);
    HEX3 : out std_logic_vector(6 downto 0);
    HEX4 : out std_logic_vector(6 downto 0);
    HEX5 : out std_logic_vector(6 downto 0);
    KEY : in std_logic_vector(3 downto 0);
    LEDR : out std_logic_vector(9 downto 0);

    -- PS2 "3.3-V LVTTL"
    PS2_CLK : inout std_logic;
    PS2_CLK2 : inout std_logic;
    PS2_DAT : inout std_logic;
    PS2_DAT2 : inout std_logic;

    -- SD "3.3-V LVTTL"
    SD_CLK : out std_logic;
    SD_CMD : inout std_logic;
    SD_DATA : inout std_logic_vector(3 downto 0);

    -- SW "3.3-V LVTTL"
    SW : in std_logic_vector(9 downto 0);

    -- VGA  "3.3-V LVTTL"
    VGA_R : out std_logic_vector(3 downto 0);
    VGA_G : out std_logic_vector(3 downto 0);
    VGA_B : out std_logic_vector(3 downto 0);
    VGA_HS : out std_logic;
    VGA_VS : out std_logic
  );
end toplevel;

architecture rtl of toplevel is
  signal s_system_rst : std_logic := '1';

  signal s_pll_locked : std_logic;
  signal s_global_async_rst : std_logic;

  signal s_cpu_rst : std_logic;
  signal s_cpu_clk : std_logic;

  signal s_vga_rst : std_logic;
  signal s_vga_clk : std_logic;
  signal s_vga_r : std_logic_vector(7 downto 0);
  signal s_vga_g : std_logic_vector(7 downto 0);
  signal s_vga_b : std_logic_vector(7 downto 0);
  signal s_vga_hs : std_logic;
  signal s_vga_vs : std_logic;

  signal s_io_in : std_logic_vector(31 downto 0);
  signal s_io_out : std_logic_vector(31 downto 0);
begin
  -- System reset signal.
  process(CLOCK_50, RESET_N)
  begin
    if RESET_N = '0' then
      s_system_rst <= '1';
    elsif rising_edge(CLOCK_50) then
      if RESET_N = '1' then
        s_system_rst <= '0';
      end if;
    end if;
  end process;

  -- Clock signals.
  pll_1: entity work.pll
    generic map (
      -- The input clock of the DE0-CV is 50 MHz.
      REFERENCE_CLOCK_FREQUENCY => "50 MHz",

      NUMBER_OF_CLOCKS => 2,

      -- The CPU clock frequency.
      OUTPUT_CLOCK_FREQUENCY0 => "70.0 MHz",

      -- Pixel frequency for HD 1280x720 @ 60 Hz = 74.250 MHz, rounded to the
      -- nearest valid frequency that the PLL can generate: 74.375 MHz (60.1 FPS).
      -- (60 Hz: 74.250 MHz, 59.94 Hz: 74.17575 MHz)
      -- TODO(m): Run the VGA logic at twice that frequency (rounded to
      -- 148.75 MHz), in order to enable two layers.
      OUTPUT_CLOCK_FREQUENCY1 => "74.375 MHz"
    )
    port map
    (
      i_rst	=> s_system_rst,
      i_refclk => CLOCK_50,
      o_clk0 => s_cpu_clk,
      o_clk1 => s_vga_clk,
      o_locked => s_pll_locked
    );

  -- Reset logic - synchronize the reset signal to the different clock domains.
  s_global_async_rst <= s_system_rst or (not s_pll_locked);

  reset_conditioner_cpu: entity work.reset_conditioner
    port map (
      i_clk => s_cpu_clk,
      i_async_rst => s_global_async_rst,
      o_rst => s_cpu_rst
    );

  reset_conditioner_vga: entity work.reset_conditioner
    port map (
      i_clk => s_vga_clk,
      i_async_rst => s_global_async_rst,
      o_rst => s_vga_rst
    );

  -- Instantiate the MC1 machine.
  mc1_1: entity work.mc1
    port map (
      -- Control signals.
      i_cpu_rst => s_cpu_rst,
      i_cpu_clk => s_cpu_clk,

      -- VGA interface.
      i_vga_rst => s_vga_rst,
      i_vga_clk => s_vga_clk,
      o_vga_r => s_vga_r,
      o_vga_g => s_vga_g,
      o_vga_b => s_vga_b,
      o_vga_hs => s_vga_hs,
      o_vga_vs => s_vga_vs,

      -- LEDs and buttons interfaces.
      -- TODO(m): Make something better here...
      i_io => s_io_in,
      o_io => s_io_out
    );

  -- VGA interface.
  -- TODO(m): Apply dithering.
  VGA_R <= s_vga_r(7 downto 4);
  VGA_G <= s_vga_g(7 downto 4);
  VGA_B <= s_vga_b(7 downto 4);
  VGA_HS <= s_vga_hs;
  VGA_VS <= s_vga_vs;

  -- I/O: Input.
  s_io_in(31 downto 14) <= (others => '0');
  s_io_in(13 downto 4) <= SW;
  s_io_in(3 downto 0) <= KEY;

  -- I/O: Output.
  LEDR <= s_io_out(16 downto 7);
  HEX0 <= s_io_out(6 downto 0);

  -- Test...
  HEX5 <= "0001001";  -- H
  HEX4 <= "0000110";  -- E
  HEX3 <= "1000111";  -- L
  HEX2 <= "1000111";  -- L
  HEX1 <= "1000000";  -- O
end rtl;
