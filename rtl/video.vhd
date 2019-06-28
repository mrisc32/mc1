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
begin
  -- TODO(m): Implement me!
  o_read_adr <= (others => '0');
  o_r <= (others => '0');
  o_g <= (others => '0');
  o_b <= (others => '0');
  o_active <= '1';
  o_hsync <= '0';
  o_vsync <= '0';
end rtl;
