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

entity vid_raster is
  generic(
    WIDTH : positive := 1280;
    HEIGHT : positive := 720;

    FRONT_PORCH_H : positive := 110;
    SYNC_WIDTH_H : positive := 40;
    BACK_PORCH_H : positive := 220;

    FRONT_PORCH_V : positive := 5;
    SYNC_WIDTH_V : positive := 5;
    BACK_PORCH_V : positive := 20;

    X_COORD_BITS : positive := 11;  -- Number of bits required for representing an x coordinate.
    Y_COORD_BITS : positive := 10   -- Number of bits required for representing an y coordinate.
  );
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;

    o_x_pos : out std_logic_vector(X_COORD_BITS-1 downto 0);
    o_y_pos : out std_logic_vector(Y_COORD_BITS-1 downto 0);

    o_hsync : out std_logic;
    o_vsync : out std_logic;

    o_active : out std_logic;
    o_pixel_phase : out std_logic
  );
end vid_raster;

architecture rtl of vid_raster is
  constant C_X_SYNC_START : positive := FRONT_PORCH_H;
  constant C_X_SYNC_END : positive := FRONT_PORCH_H + SYNC_WIDTH_H;
  constant C_X_ACTIVE_START : positive := FRONT_PORCH_H + SYNC_WIDTH_H + BACK_PORCH_H;
  constant C_X_ACTIVE_END : positive := FRONT_PORCH_H + SYNC_WIDTH_H + BACK_PORCH_H + WIDTH;

  constant C_Y_SYNC_START : positive := FRONT_PORCH_V;
  constant C_Y_SYNC_END : positive := FRONT_PORCH_V + SYNC_WIDTH_V;
  constant C_Y_ACTIVE_START : positive := FRONT_PORCH_V + SYNC_WIDTH_V + BACK_PORCH_V;
  constant C_Y_ACTIVE_END : positive := FRONT_PORCH_V + SYNC_WIDTH_V + BACK_PORCH_V + HEIGHT;

  signal s_x_pos : unsigned(X_COORD_BITS-1 downto 0);
  signal s_y_pos : unsigned(Y_COORD_BITS-1 downto 0);
  signal s_hsync : std_logic;
  signal s_vsync : std_logic;
  signal s_hactive : std_logic;
  signal s_vactive : std_logic;
  signal s_pixel_phase : std_logic;
begin
  process(i_clk, i_rst)
    variable v_x_pos : unsigned(X_COORD_BITS-1 downto 0);
    variable v_y_pos : unsigned(Y_COORD_BITS-1 downto 0);
    variable v_hsync : std_logic;
    variable v_vsync : std_logic;
    variable v_hactive : std_logic;
    variable v_vactive : std_logic;
  begin
    if i_rst = '1' then
      s_y_pos <= (others => '0');
      s_x_pos <= (others => '0');
      s_hsync <= '0';
      s_vsync <= '0';
      s_hactive <= '0';
      s_vactive <= '0';
      s_pixel_phase <= '0';
    elsif rising_edge(i_clk) then
      -- We only update the raster state on every second clock cycle, since
      -- each pixel is two clock cycles wide.
      if s_pixel_phase = '1' then
        v_x_pos := s_x_pos;
        v_y_pos := s_y_pos;
        v_hsync := s_hsync;
        v_vsync := s_vsync;
        v_hactive := s_hactive;
        v_vactive := s_vactive;

        if v_x_pos = C_X_ACTIVE_END then
          -- End of line reached. Restart the horizontal raster.
          v_x_pos := to_unsigned(0, X_COORD_BITS);
          v_hactive := '0';

          if v_y_pos = C_Y_ACTIVE_END then
            -- End of frame reached. Restart the vertical raster.
            v_y_pos := to_unsigned(0, Y_COORD_BITS);
            v_vactive := '0';
          else
            if v_y_pos = C_Y_SYNC_START then
              v_vsync := '1';
            elsif v_y_pos = C_Y_SYNC_END then
              v_vsync := '0';
            elsif v_y_pos = C_Y_ACTIVE_START then
              v_vactive := '1';
            end if;
            v_y_pos := v_y_pos + to_unsigned(1, Y_COORD_BITS);
          end if;
        else
          if v_x_pos = C_X_SYNC_START then
            v_hsync := '1';
          elsif v_x_pos = C_X_SYNC_END then
            v_hsync := '0';
          elsif v_x_pos = C_X_ACTIVE_START then
            v_hactive := '1';
          end if;
          v_x_pos := v_x_pos + to_unsigned(1, X_COORD_BITS);
        end if;

        -- Update the state signals.
        s_x_pos <= v_x_pos;
        s_y_pos <= v_y_pos;
        s_hsync <= v_hsync;
        s_vsync <= v_vsync;
        s_hactive <= v_hactive;
        s_vactive <= v_vactive;
      end if;

      -- Next pixel phase...
      s_pixel_phase <= not s_pixel_phase;
    end if;
  end process;

  -- Outputs.
  o_x_pos <= std_logic_vector(s_x_pos);
  o_y_pos <= std_logic_vector(s_y_pos);
  o_hsync <= s_hsync;
  o_vsync <= s_vsync;
  o_active <= s_hactive and s_vactive;
  o_pixel_phase <= s_pixel_phase;
end rtl;
