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
-- This is the top level entity for Basys3.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use work.mmio_types.all;
use work.vid_types.all;

entity toplevel is
  port(
    -- Clock
    clk : in std_logic;

    -- 7-segment
    -- TODO(m): Add these

    -- Switches
    sw : in std_logic_vector(15 downto 0);

    -- Buttons
    btnC : in std_logic;
    btnU : in std_logic;
    btnL : in std_logic;
    btnR : in std_logic;
    btnD : in std_logic;

    -- LEDs
    led : out std_logic_vector(15 downto 0);

    -- VGA  "3.3-V LVTTL"
    vgaRed : out std_logic_vector(3 downto 0);
    vgaGreen : out std_logic_vector(3 downto 0);
    vgaBlue : out std_logic_vector(3 downto 0);
    Hsync : out std_logic;
    Vsync : out std_logic
  );
end toplevel;

architecture rtl of toplevel is
  signal s_reset_n : std_logic;
  signal s_system_rst : std_logic;

  signal s_cpu_pll_locked : std_logic;
  signal s_vga_pll_locked : std_logic;
  signal s_global_async_rst : std_logic;

  signal s_cpu_rst : std_logic;
  signal s_cpu_clk : std_logic;

  signal s_vga_rst : std_logic;
  signal s_vga_clk : std_logic;

  signal s_io_switches : std_logic_vector(31 downto 0);
  signal s_io_buttons : std_logic_vector(31 downto 0);
  signal s_io_regs_w : T_MMIO_REGS_WO;
begin
  -- We use btnC as reset.
  s_reset_n <= not btnU;

  -- System reset signal: This is the reset signal from the board. The stabilizer guarantees that
  -- the reset signal will be held high for a certain period.
  reset_stabilizer_1: entity work.reset_stabilizer
    generic map (
      STABLE_COUNT_BITS => 23  -- Hold reset high for 2^23 100 MHz cycles (84 ms).
    )
    port map (
      i_rst_n => s_reset_n,
      i_clk => clk,
      o_rst => s_system_rst
    );

  -- Generate the CPU clock signal.
  pll_1 : entity work.pll_cpu
    port map (
      reset => s_system_rst,
      clk_in1 => clk,
      clk_out1 => s_cpu_clk,
      locked => s_cpu_pll_locked
    );

  -- Generate the VGA clock signal.
  pll_2 : entity work.pll_vga
    port map (
      reset => s_system_rst,
      clk_in1 => clk,
      clk_out1 => s_vga_clk,
      locked => s_vga_pll_locked
    );

  -- Reset logic - synchronize the reset signal to the different clock domains.
  s_global_async_rst <= s_system_rst or (not (s_cpu_pll_locked and s_vga_pll_locked));

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
      COLOR_BITS => vgaRed'length,
      LOG2_VRAM_SIZE => 15,         -- 4*2^15 = 128 KiB
      VIDEO_CONFIG => C_1920_1080
    )
    port map (
      -- Control signals.
      i_cpu_rst => s_cpu_rst,
      i_cpu_clk => s_cpu_clk,

      -- VGA interface.
      i_vga_rst => s_vga_rst,
      i_vga_clk => s_vga_clk,
      o_vga_r => vgaRed,
      o_vga_g => vgaGreen,
      o_vga_b => vgaBlue,
      o_vga_hs => Hsync,
      o_vga_vs => Vsync,

      -- I/O registers.
      i_io_switches => s_io_switches,
      i_io_buttons => s_io_buttons,
      o_io_regs_w => s_io_regs_w
    );

  -- I/O: Input.
  s_io_switches(31 downto 16) <= (others => '0');
  s_io_switches(15 downto 0) <= sw;
  s_io_buttons(31 downto 4) <= (others => '0');
  s_io_buttons(0) <= btnU;
  s_io_buttons(1) <= btnL;
  s_io_buttons(2) <= btnR;
  s_io_buttons(3) <= btnD;

  -- I/O: Output.
  led <= s_io_regs_w.LEDS(15 downto 0);
end rtl;
