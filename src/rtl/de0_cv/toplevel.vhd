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
use work.mmio_types.all;
use work.vid_types.all;

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

  signal s_pll_cpu_locked : std_logic;
  signal s_pll_vga_locked : std_logic;
  signal s_global_async_rst : std_logic;

  signal s_cpu_rst : std_logic;
  signal s_cpu_clk : std_logic;

  signal s_vga_rst : std_logic;
  signal s_vga_clk : std_logic;
  signal s_vga_r : std_logic_vector(3 downto 0);
  signal s_vga_g : std_logic_vector(3 downto 0);
  signal s_vga_b : std_logic_vector(3 downto 0);
  signal s_vga_hs : std_logic;
  signal s_vga_vs : std_logic;

  signal s_io_switches : std_logic_vector(31 downto 0);
  signal s_io_buttons : std_logic_vector(31 downto 0);
  signal s_io_regs_w : T_MMIO_REGS_WO;
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

  -- Generate the CPU clock signal.
  -- 70 MHz seems to be a good safe bet, but going higher is certainly possible.
  pll_cpu: entity work.pll
    generic map (
      REFERENCE_CLOCK_FREQUENCY => "50 MHz",
      NUMBER_OF_CLOCKS => 1,
      OUTPUT_CLOCK_FREQUENCY0 => "70.0 MHz"
    )
    port map
    (
      i_rst => s_system_rst,
      i_refclk => CLOCK_50,
      o_clk0 => s_cpu_clk,
      o_locked => s_pll_cpu_locked
    );

  -- Generate the VGA clock signal.
  -- Pixel frequencies for supported video modes:
  --  1920x1080 @ 60 Hz: 148.500 MHz (rounded to 148.4375 MHz, 59.97 FPS)
  --   1280x720 @ 60 Hz:  74.250 MHz (rounded to 74.242424 MHz, 59.99 FPS)
  --    800x600 @ 60 Hz:  40.000 MHz
  --    640x480 @ 60 Hz:  25.175 MHz (rounded to 25.175644 MHz, 60.002 FPS)
  pll_vga: entity work.pll
    generic map (
      REFERENCE_CLOCK_FREQUENCY => "50 MHz",
      NUMBER_OF_CLOCKS => 1,
      OUTPUT_CLOCK_FREQUENCY0 => "148.4375 MHz"
    )
    port map
    (
      i_rst => s_system_rst,
      i_refclk => CLOCK2_50,
      o_clk0 => s_vga_clk,
      o_locked => s_pll_vga_locked
    );

  -- Reset logic - synchronize the reset signal to the different clock domains.
  s_global_async_rst <= s_system_rst or (not (s_pll_cpu_locked and s_pll_vga_locked));

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
    generic map (
      COLOR_BITS => s_vga_r'length,
      LOG2_VRAM_SIZE => 16,          -- 4*2^16 = 256 KiB
      VIDEO_CONFIG => C_1920_1080
    )
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

      -- I/O registers.
      i_io_switches => s_io_switches,
      i_io_buttons => s_io_buttons,
      o_io_regs_w => s_io_regs_w
    );

  -- VGA interface.
  VGA_R <= s_vga_r;
  VGA_G <= s_vga_g;
  VGA_B <= s_vga_b;
  VGA_HS <= s_vga_hs;
  VGA_VS <= s_vga_vs;

  -- I/O: Input.
  s_io_switches(31 downto 10) <= (others => '0');
  s_io_switches(9 downto 0) <= SW;
  s_io_buttons(31 downto 4) <= (others => '0');
  s_io_buttons(3 downto 0) <= KEY;

  -- I/O: Output.
  HEX0 <= not s_io_regs_w.SEGDISP0(6 downto 0);
  HEX1 <= not s_io_regs_w.SEGDISP1(6 downto 0);
  HEX2 <= not s_io_regs_w.SEGDISP2(6 downto 0);
  HEX3 <= not s_io_regs_w.SEGDISP3(6 downto 0);
  HEX4 <= not s_io_regs_w.SEGDISP4(6 downto 0);
  HEX5 <= not s_io_regs_w.SEGDISP5(6 downto 0);
  LEDR <= s_io_regs_w.LEDS(9 downto 0);
end rtl;
