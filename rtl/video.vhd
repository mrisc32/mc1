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
  -- TODO(m): Make this configurable.
  constant C_FRAMEBUFFER0_START : unsigned(ADR_BITS-1 downto 0) := to_unsigned(32768, ADR_BITS);
  constant C_FRAMEBUFFER1_START : unsigned(ADR_BITS-1 downto 0) := to_unsigned(49152, ADR_BITS);

  signal s_layer0_adr : unsigned(ADR_BITS-1 downto 0);
  signal s_layer0_byte_no : unsigned(1 downto 0);

  signal s_layer0_word : std_logic_vector(31 downto 0);

  signal s_raster_x : std_logic_vector(10 downto 0);
  signal s_raster_y : std_logic_vector(9 downto 0);
  signal s_hsync : std_logic;
  signal s_vsync : std_logic;
  signal s_active : std_logic;
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
      o_active => s_active
    );

  -- Video memory read logic.
  -- TODO(m): This is a hard-coded, simplified version. Implement a proper VCU.
  process(i_clk)
  begin
    if rising_edge(i_clk) then
      if s_vsync = '1' then
        s_layer0_adr <= C_FRAMEBUFFER0_START;
        s_layer0_byte_no <= to_unsigned(0, s_layer0_byte_no'length);
      elsif s_active = '1' then
        -- Update the pixel read addresses.
        if s_layer0_byte_no = 2x"3" then
          s_layer0_adr <= s_layer0_adr + to_unsigned(1, ADR_BITS);
        end if;
        s_layer0_byte_no <= s_layer0_byte_no + to_unsigned(1, s_layer0_byte_no'length);
      end if;
    end if;
  end process;

  -- RAM read logic.
  process(i_clk)
  begin
    if rising_edge(i_clk) then
      o_read_adr <= std_logic_vector(s_layer0_adr);
      s_layer0_word <= i_read_dat;
    end if;
  end process;

  -- Layer combine logic.
  -- TODO(m): Implement me!
  -- HACK!!!!
  o_r <= s_layer0_word(24 downto 17);
  o_g <= s_layer0_word(14 downto 7);
  o_b <= s_layer0_word(7 downto 0);

  o_hsync <= s_hsync;
  o_vsync <= s_vsync;
end rtl;
