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
-- This is the top level entity of the MC1.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity mc1 is
  port(
    -- CPU interface.
    i_cpu_rst : in std_logic;
    i_cpu_clk : in std_logic;

    -- VGA interface.
    i_vga_rst : in std_logic;
    i_vga_clk : in std_logic;
    o_vga_r : out std_logic_vector(7 downto 0);
    o_vga_g : out std_logic_vector(7 downto 0);
    o_vga_b : out std_logic_vector(7 downto 0);
    o_vga_hs : out std_logic;
    o_vga_vs : out std_logic
  );
end mc1;

architecture rtl of mc1 is
  -- RAM size (log2 of number of 32-bit words).
  constant C_LOG2_RAM_SIZE : positive := 16;

  -- CPU memory interface (Wishbone B4 pipelined master).
  signal s_cpu_cyc : std_logic;
  signal s_cpu_stb : std_logic;
  signal s_cpu_adr : std_logic_vector(31 downto 2);
  signal s_cpu_dat_w : std_logic_vector(31 downto 0);
  signal s_cpu_we : std_logic;
  signal s_cpu_sel : std_logic_vector(3 downto 0);
  signal s_cpu_dat : std_logic_vector(31 downto 0);
  signal s_cpu_ack : std_logic;
  signal s_cpu_stall : std_logic;
  signal s_cpu_err : std_logic;

  -- Video logic memory interface.
  signal s_video_adr : std_logic_vector(C_LOG2_RAM_SIZE-1 downto 0);
  signal s_video_dat : std_logic_vector(31 downto 0);
  signal s_video_r : std_logic_vector(7 downto 0);
  signal s_video_g : std_logic_vector(7 downto 0);
  signal s_video_b : std_logic_vector(7 downto 0);
  signal s_video_hsync : std_logic;
  signal s_video_vsync : std_logic;
begin
  -- Instantiate the CPU core.
  mrisc32_core_1: entity work.core
    port map (
      i_clk => i_cpu_clk,
      i_rst => i_cpu_rst,

      -- Data interface.
      o_wb_cyc => s_cpu_cyc,
      o_wb_stb => s_cpu_stb,
      o_wb_adr => s_cpu_adr,
      o_wb_dat => s_cpu_dat_w,
      o_wb_we => s_cpu_we,
      o_wb_sel => s_cpu_sel,
      i_wb_dat => s_cpu_dat,
      i_wb_ack => s_cpu_ack,
      i_wb_stall => s_cpu_stall,
      i_wb_err => s_cpu_err
    );

  -- Instantiate the Wishbone memory subsystem.
  ram_1: entity work.ram
    generic map (
      ADR_BITS => C_LOG2_RAM_SIZE
    )
    port map (
      i_rst => i_cpu_rst,

      -- CPU interface.
      i_wb_clk => i_cpu_clk,
      i_wb_cyc => s_cpu_cyc,
      i_wb_stb => s_cpu_stb,
      i_wb_adr => s_cpu_adr(C_LOG2_RAM_SIZE+1 downto 2),
      i_wb_dat => s_cpu_dat_w,
      i_wb_we => s_cpu_we,
      i_wb_sel => s_cpu_sel,
      o_wb_dat => s_cpu_dat,
      o_wb_ack => s_cpu_ack,
      o_wb_stall => s_cpu_stall,

      -- Video interface.
      i_read_clk => i_vga_clk,
      i_read_adr => s_video_adr,
      o_read_dat => s_video_dat
    );
  s_cpu_err <= '0';

  -- Instantiate the video logic.
  video_1: entity work.video
    generic map (
      ADR_BITS => C_LOG2_RAM_SIZE
    )
    port map (
      i_rst => i_vga_rst,
      i_clk => i_vga_clk,

      o_read_adr => s_video_adr,
      i_read_dat => s_video_dat,

      o_r => s_video_r,
      o_g => s_video_g,
      o_b => s_video_b,

      o_hsync => s_video_hsync,
      o_vsync => s_video_vsync
    );

  o_vga_r <= s_video_r;
  o_vga_g <= s_video_g;
  o_vga_b <= s_video_b;
  o_vga_hs <= s_video_hsync;
  o_vga_vs <= s_video_vsync;
end rtl;
