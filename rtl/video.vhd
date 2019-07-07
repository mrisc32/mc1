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

    o_active : out std_logic;
    o_hsync : out std_logic;
    o_vsync : out std_logic
  );
end video;

architecture rtl of video is
  signal s_dummy_adr : std_logic_vector(ADR_BITS-1 downto 0);
  signal s_next_dummy_adr : std_logic_vector(ADR_BITS-1 downto 0);

  signal s_x_pos : std_logic_vector(10 downto 0);
  signal s_y_pos : std_logic_vector(9 downto 0);
  signal s_pixel_phase : std_logic;
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
      X_COORD_BITS => s_x_pos'length,
      Y_COORD_BITS => s_y_pos'length
    )
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      o_x_pos => s_x_pos,
      o_y_pos => s_y_pos,
      o_hsync => o_hsync,
      o_vsync => o_vsync,
      o_active => o_active,
      o_pixel_phase => s_pixel_phase
    );

  -- Frame buffer read-out.
  -- TODO(m): Implement me!
  -- Right now we just output some random values by addressing all the RAM.
  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      s_dummy_adr <= (others => '0');
      o_r <= (others => '0');
      o_g <= (others => '0');
      o_b <= (others => '0');
    elsif rising_edge(i_clk) then
      s_dummy_adr <= s_next_dummy_adr;
      o_r <= i_read_dat(23 downto 16);
      o_g <= i_read_dat(15 downto 8);
      o_b <= i_read_dat(7 downto 0);
    end if;
  end process;

  o_read_adr <= s_dummy_adr;

  -- Calculate the next dummy address.
  s_next_dummy_adr <= std_logic_vector(unsigned(s_dummy_adr) + 1);
end rtl;
