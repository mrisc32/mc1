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

    X_COORD_BITS : positive := 12;  -- Number of bits required for representing an x coordinate.
    Y_COORD_BITS : positive := 11   -- Number of bits required for representing a y coordinate.
  );
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;

    o_x_pos : out std_logic_vector(X_COORD_BITS-1 downto 0);
    o_y_pos : out std_logic_vector(Y_COORD_BITS-1 downto 0);

    o_hsync : out std_logic;
    o_vsync : out std_logic;
    o_restart_frame : out std_logic
  );
end vid_raster;

architecture rtl of vid_raster is
  constant C_X_START : integer := -(FRONT_PORCH_H + SYNC_WIDTH_H + BACK_PORCH_H);
  constant C_X_SYNC_START : integer := -(SYNC_WIDTH_H + BACK_PORCH_H);
  constant C_X_SYNC_END : integer := -BACK_PORCH_H;
  constant C_X_END : integer := WIDTH;

  constant C_Y_START : integer := -(FRONT_PORCH_V + SYNC_WIDTH_V + BACK_PORCH_V);
  constant C_Y_SYNC_START : integer := -(SYNC_WIDTH_V + BACK_PORCH_V);
  constant C_Y_SYNC_END : integer := -BACK_PORCH_V;
  constant C_Y_END : integer := HEIGHT;

  signal s_x_pos : signed(X_COORD_BITS-1 downto 0);
  signal s_y_pos : signed(Y_COORD_BITS-1 downto 0);
  signal s_hsync : std_logic;
  signal s_vsync : std_logic;
  signal s_restart_frame : std_logic;
begin
  process(i_clk, i_rst)
    variable v_x_pos : signed(X_COORD_BITS-1 downto 0);
    variable v_y_pos : signed(Y_COORD_BITS-1 downto 0);
    variable v_hsync : std_logic;
    variable v_vsync : std_logic;
    variable v_restart_frame : std_logic;
  begin
    if i_rst = '1' then
      s_y_pos <= (others => '0');
      s_x_pos <= (others => '0');
      s_hsync <= '0';
      s_vsync <= '0';
      s_restart_frame <= '1';
    elsif rising_edge(i_clk) then
      v_x_pos := s_x_pos;
      v_y_pos := s_y_pos;
      v_hsync := s_hsync;
      v_vsync := s_vsync;
      v_restart_frame := '0';

      if v_x_pos = C_X_END then
        -- End of line reached. Restart the horizontal raster.
        v_x_pos := to_signed(C_X_START, X_COORD_BITS);

        if v_y_pos = C_Y_END then
          -- End of frame reached. Restart the vertical raster.
          v_y_pos := to_signed(C_Y_START, Y_COORD_BITS);
          v_restart_frame := '1';
        else
          if v_y_pos = C_Y_SYNC_START then
            v_vsync := '1';
          elsif v_y_pos = C_Y_SYNC_END then
            v_vsync := '0';
          end if;
          v_y_pos := v_y_pos + to_signed(1, Y_COORD_BITS);
        end if;
      else
        if v_x_pos = C_X_SYNC_START then
          v_hsync := '1';
        elsif v_x_pos = C_X_SYNC_END then
          v_hsync := '0';
        end if;
        v_x_pos := v_x_pos + to_signed(1, X_COORD_BITS);
      end if;

      -- Update the state signals.
      s_x_pos <= v_x_pos;
      s_y_pos <= v_y_pos;
      s_hsync <= v_hsync;
      s_vsync <= v_vsync;
      s_restart_frame <= v_restart_frame;
    end if;
  end process;

  -- Outputs.
  o_x_pos <= std_logic_vector(s_x_pos);
  o_y_pos <= std_logic_vector(s_y_pos);
  o_hsync <= s_hsync;
  o_vsync <= s_vsync;
  o_restart_frame <= s_restart_frame;
end rtl;
