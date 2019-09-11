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

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.vid_types.all;

entity video is
  generic(
    ADR_BITS : positive := 16;

    WIDTH : positive := 1280;
    HEIGHT : positive := 720;

    FRONT_PORCH_H : positive := 110;
    SYNC_WIDTH_H : positive := 40;
    BACK_PORCH_H : positive := 220;

    FRONT_PORCH_V : positive := 5;
    SYNC_WIDTH_V : positive := 5;
    BACK_PORCH_V : positive := 20
  );
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;

    o_read_adr : out std_logic_vector(ADR_BITS-1 downto 0);
    i_read_dat : in std_logic_vector(31 downto 0);

    o_r : out std_logic_vector(7 downto 0);
    o_g : out std_logic_vector(7 downto 0);
    o_b : out std_logic_vector(7 downto 0);

    o_hsync : out std_logic;
    o_vsync : out std_logic
  );
end video;

architecture rtl of video is
  -- Number of cycles to delay the sync output signals, due to the pipeline delay
  -- of the pixel pipeline.
  constant C_SYNC_DELAY : positive := 5;
  signal s_hsync_delayed : std_logic_vector(C_SYNC_DELAY-1 downto 0);
  signal s_vsync_delayed : std_logic_vector(C_SYNC_DELAY-1 downto 0);

  signal s_raster_x : std_logic_vector(11 downto 0);
  signal s_raster_y : std_logic_vector(10 downto 0);
  signal s_hsync : std_logic;
  signal s_vsync : std_logic;
  signal s_restart_frame : std_logic;

  signal s_vcpp_mem_read_en : std_logic;
  signal s_vcpp_mem_read_addr : std_logic_vector(23 downto 0);
  signal s_next_vcpp_mem_ack : std_logic;
  signal s_vcpp_mem_ack : std_logic;
  signal s_vcpp_reg_write_enable : std_logic;
  signal s_vcpp_pal_write_enable : std_logic;
  signal s_vcpp_write_addr : std_logic_vector(7 downto 0);
  signal s_vcpp_write_data : std_logic_vector(31 downto 0);

  signal s_regs : T_VID_REGS;

  signal s_pix_mem_read_en : std_logic;
  signal s_pix_mem_read_addr : std_logic_vector(23 downto 0);
  signal s_next_pix_mem_ack : std_logic;
  signal s_pix_mem_ack : std_logic;
  signal s_pix_pal_addr : std_logic_vector(7 downto 0);
  signal s_pix_pal_data : std_logic_vector(31 downto 0);
  signal s_pix_color : std_logic_vector(31 downto 0);
begin
  -- Instantiate the raster control unit.
  rcu_1: entity work.vid_raster
    generic map (
      WIDTH => WIDTH,
      HEIGHT => HEIGHT,
      FRONT_PORCH_H => FRONT_PORCH_H,
      SYNC_WIDTH_H => SYNC_WIDTH_H,
      BACK_PORCH_H => BACK_PORCH_H,
      FRONT_PORCH_V => FRONT_PORCH_V,
      SYNC_WIDTH_V => SYNC_WIDTH_V,
      BACK_PORCH_V => BACK_PORCH_V,
      X_COORD_BITS => s_raster_x'length,
      Y_COORD_BITS => s_raster_y'length
    )
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      o_x_pos => s_raster_x,
      o_y_pos => s_raster_y,
      o_hsync => s_hsync,
      o_vsync => s_vsync,
      o_restart_frame => s_restart_frame
    );

  -- Instantiate the video control program processor.
  vcpp_1: entity work.vid_vcpp
    generic map (
      Y_COORD_BITS => s_raster_y'length
    )
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      i_restart_frame => s_restart_frame,
      i_raster_y => s_raster_y,
      o_mem_read_en => s_vcpp_mem_read_en,
      o_mem_read_addr => s_vcpp_mem_read_addr,
      i_mem_data => i_read_dat,
      i_mem_ack => s_vcpp_mem_ack,
      o_reg_write_enable => s_vcpp_reg_write_enable,
      o_pal_write_enable => s_vcpp_pal_write_enable,
      o_write_addr => s_vcpp_write_addr,
      o_write_data => s_vcpp_write_data
    );

  -- Instantiate the video control registers.
  vcr_1: entity work.vid_regs
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      i_write_enable => s_vcpp_reg_write_enable,
      i_write_addr => s_vcpp_write_addr(2 downto 0),
      i_write_data => s_vcpp_write_data(23 downto 0),
      o_regs => s_regs
    );

  -- Instantiate the video palette.
  palette_1: entity work.vid_palette
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      i_write_enable => s_vcpp_pal_write_enable,
      i_write_addr => s_vcpp_write_addr,
      i_write_data => s_vcpp_write_data,
      i_read_addr => s_pix_pal_addr,
      o_read_data => s_pix_pal_data
    );

  -- Instantiate the pixel pipeline.
  pixel_pipe_1: entity work.vid_pixel
    generic map (
      X_COORD_BITS => s_raster_x'length
    )
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      i_raster_x => s_raster_x,
      o_mem_read_en => s_pix_mem_read_en,
      o_mem_read_addr => s_pix_mem_read_addr,
      i_mem_data => i_read_dat,
      i_mem_ack => s_pix_mem_ack,
      o_pal_addr => s_pix_pal_addr,
      i_pal_data => s_pix_pal_data,
      i_regs => s_regs,
      o_color => s_pix_color
    );

  --------------------------------------------------------------------------------------------------
  -- VRAM read logic - only one entity may access VRAM during each clock cycle.
  --------------------------------------------------------------------------------------------------

  -- Select the active read unit - The pixel pipe has priority over the VCPP.
  o_read_adr <= s_pix_mem_read_addr(ADR_BITS-1 downto 0) when s_pix_mem_read_en = '1' else
                s_vcpp_mem_read_addr(ADR_BITS-1 downto 0) when s_vcpp_mem_read_en = '1' else
                (others => '-');
  s_next_pix_mem_ack <= s_pix_mem_read_en;
  s_next_vcpp_mem_ack <= s_vcpp_mem_read_en and not s_pix_mem_read_en;

  -- Respond with an ack to the relevant unit (one cycle after).
  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      s_pix_mem_ack <= '0';
      s_vcpp_mem_ack <= '0';
    elsif rising_edge(i_clk) then
      s_pix_mem_ack <= s_next_pix_mem_ack;
      s_vcpp_mem_ack <= s_next_vcpp_mem_ack;
    end if;
  end process;

  -- RGB output from the pixel pipeline.
  -- The internal color format is ABGR32 (little endian):
  --   |AAAAAAAA|BBBBBBBB|GGGGGGGG|RRRRRRRR|
  o_r <= s_pix_color(7 downto 0);
  o_g <= s_pix_color(15 downto 8);
  o_b <= s_pix_color(23 downto 16);

  -- Horizontal and vertical sync signal outputs.
  -- These need to be cycle-delayed in order to be in sync with the color
  -- output from the pixel pipeline.
  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      s_hsync_delayed <= (others => '0');
      s_vsync_delayed <= (others => '0');
    elsif rising_edge(i_clk) then
      s_hsync_delayed(0) <= s_hsync;
      s_vsync_delayed(0) <= s_vsync;
      for k in 1 to C_SYNC_DELAY-1 loop
        s_hsync_delayed(k) <= s_hsync_delayed(k-1);
        s_vsync_delayed(k) <= s_vsync_delayed(k-1);
      end loop;
    end if;
  end process;
  o_hsync <= s_hsync_delayed(C_SYNC_DELAY-1);
  o_vsync <= s_vsync_delayed(C_SYNC_DELAY-1);
end rtl;
