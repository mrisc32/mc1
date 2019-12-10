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
    COLOR_BITS : positive;
    ADR_BITS : positive;
    VIDEO_CONFIG : T_VIDEO_CONFIG
  );
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;

    o_read_adr : out std_logic_vector(ADR_BITS-1 downto 0);
    i_read_dat : in std_logic_vector(31 downto 0);

    o_r : out std_logic_vector(COLOR_BITS-1 downto 0);
    o_g : out std_logic_vector(COLOR_BITS-1 downto 0);
    o_b : out std_logic_vector(COLOR_BITS-1 downto 0);

    o_hsync : out std_logic;
    o_vsync : out std_logic;

    o_restart_frame : out std_logic;
    o_raster_x : out std_logic_vector(15 downto 0);
    o_raster_y : out std_logic_vector(15 downto 0)
  );
end video;

architecture rtl of video is
  -- Should we enable dithering or not?
  function ENABLE_DITHERING return boolean is
  begin
    if (COLOR_BITS < 8) then
      return true;
    else
      return false;
    end if;
  end;

  -- Number of cycles to delay the sync output signals, due to color pipeline
  -- delays.
  function SYNC_DELAY return integer is
    constant C_PIXEL_DELAY : integer := 6;
    constant C_DITHER_DELAY : integer := 2;
  begin
    if ENABLE_DITHERING then
      return C_PIXEL_DELAY + C_DITHER_DELAY;
    else
      return C_PIXEL_DELAY;
    end if;
  end function;

  signal s_raster_x : std_logic_vector(11 downto 0);
  signal s_raster_y : std_logic_vector(11 downto 0);
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

  signal s_r8 : std_logic_vector(7 downto 0);
  signal s_g8 : std_logic_vector(7 downto 0);
  signal s_b8 : std_logic_vector(7 downto 0);
  signal s_dither_method : std_logic_vector(1 downto 0);

  signal s_hsync_delayed : std_logic_vector(SYNC_DELAY-1 downto 0);
  signal s_vsync_delayed : std_logic_vector(SYNC_DELAY-1 downto 0);
begin
  -- Instantiate the raster control unit.
  rcu_1: entity work.vid_raster
    generic map (
      VIDEO_CONFIG => VIDEO_CONFIG,
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
      X_COORD_BITS => s_raster_x'length,
      Y_COORD_BITS => s_raster_y'length
    )
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      i_restart_frame => s_restart_frame,
      i_raster_x => s_raster_x,
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
      i_restart_frame => s_restart_frame,
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
      X_COORD_BITS => s_raster_x'length,
      Y_COORD_BITS => s_raster_y'length
    )
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      i_raster_x => s_raster_x,
      i_raster_y => s_raster_y,
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


  --------------------------------------------------------------------------------------------------
  -- Video signal output logic.
  --------------------------------------------------------------------------------------------------

  -- Extract the R, G and B channels from the pixel pipeline output.
  -- The internal color format is ABGR32 (little endian):
  --   |AAAAAAAA|BBBBBBBB|GGGGGGGG|RRRRRRRR|
  s_r8 <= s_pix_color(7 downto 0);
  s_g8 <= s_pix_color(15 downto 8);
  s_b8 <= s_pix_color(23 downto 16);

  -- Use dithering (or not) to generate the final RGB signals.
  DitherGen: if ENABLE_DITHERING generate
  begin
    -- The dither method is controlled via the RMODE VCR.
    s_dither_method <= s_regs.RMODE(1 downto 0);

    dither1: entity work.dither
      generic map(
        BITS => COLOR_BITS
      )
      port map (
        i_rst => i_rst,
        i_clk => i_clk,
        i_method => s_dither_method,
        i_r => s_r8,
        i_g => s_g8,
        i_b => s_b8,
        o_r => o_r,
        o_g => o_g,
        o_b => o_b
      );
  else generate
    o_r <= s_r8;
    o_g <= s_g8;
    o_b <= s_b8;
  end generate;

  -- Horizontal and vertical sync signal outputs.
  -- These need to be cycle-delayed in order to be in sync with the color outputs.
  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      s_hsync_delayed <= (others => '0');
      s_vsync_delayed <= (others => '0');
    elsif rising_edge(i_clk) then
      s_hsync_delayed(0) <= s_hsync;
      s_vsync_delayed(0) <= s_vsync;
      for k in 1 to SYNC_DELAY-1 loop
        s_hsync_delayed(k) <= s_hsync_delayed(k-1);
        s_vsync_delayed(k) <= s_vsync_delayed(k-1);
      end loop;
    end if;
  end process;
  o_hsync <= s_hsync_delayed(SYNC_DELAY-1);
  o_vsync <= s_vsync_delayed(SYNC_DELAY-1);

  -- Extra output signals used for MMIO registers.
  o_restart_frame <= s_restart_frame;
  o_raster_x(s_raster_x'left downto 0) <= s_raster_x;
  o_raster_x(15 downto s_raster_x'length) <= (others => s_raster_x(s_raster_x'left));
  o_raster_y(s_raster_x'left downto 0) <= s_raster_y;
  o_raster_y(15 downto s_raster_y'length) <= (others => s_raster_y(s_raster_y'left));
end rtl;
