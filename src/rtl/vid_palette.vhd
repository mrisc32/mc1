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

----------------------------------------------------------------------------------------------------
-- Video color palette memory.
----------------------------------------------------------------------------------------------------

entity vid_palette is
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;

    i_write_enable : in std_logic;
    i_write_addr : in std_logic_vector(7 downto 0);
    i_write_data : in std_logic_vector(31 downto 0);

    i_read_addr : in std_logic_vector(7 downto 0);
    o_read_data : out std_logic_vector(31 downto 0)
  );
end vid_palette;

architecture rtl of vid_palette is
begin
  -- Note: We could use a simple dual port memory here, but this should hopefully
  -- be reduced to that by the synthesis tool anyway.
  ram_tdp_0: entity work.ram_true_dual_port
    generic map (
      DATA_BITS => 32,
      ADR_BITS => 8
    )
    port map (
      i_clk_a => i_clk,
      i_we_a => i_write_enable,
      i_adr_a => i_write_addr,
      i_data_a => i_write_data,

      i_clk_b => i_clk,
      i_we_b => '0',
      i_adr_b => i_read_addr,
      i_data_b => (others => '0'),
      o_data_b => o_read_data
    );
end rtl;
