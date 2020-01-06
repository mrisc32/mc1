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
-- This is a reset signal stabilizer (e.g. de-bouncing).
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity reset_stabilizer is
  generic(
    -- With a 50 MHz clock, a 16 bit counter will wrap roughly every 1 ms.
    STABLE_COUNT_BITS : positive := 16
  );
  port(
    i_rst_n : in std_logic;
    i_clk : in std_logic;
    o_rst : out std_logic
  );
end reset_stabilizer;

architecture rtl of reset_stabilizer is
  constant C_ALL_ONES : unsigned(STABLE_COUNT_BITS-1 downto 0) := (others => '1');

  signal s_next_stable_count : unsigned(STABLE_COUNT_BITS-1 downto 0);
  signal s_is_stable : std_logic;

  signal s_stable_rst : std_logic := '1';
  signal s_stable_count : unsigned(STABLE_COUNT_BITS-1 downto 0) := to_unsigned(0, STABLE_COUNT_BITS);
begin
  -- The signal s_is_stable will be 1 every 2^STABLE_COUNT_BITS cycles.
  s_next_stable_count <= s_stable_count + to_unsigned(1, STABLE_COUNT_BITS);
  s_is_stable <= '1' when s_stable_count = C_ALL_ONES else '0';

  process(i_clk, i_rst_n)
  begin
    if i_rst_n = '0' then
      s_stable_rst <= '1';
      s_stable_count <= to_unsigned(0, STABLE_COUNT_BITS);
    elsif rising_edge(i_clk) then
      if s_is_stable = '1' then
        s_stable_rst <= '0';
      end if;
      s_stable_count <= s_next_stable_count;
    end if;
  end process;

  o_rst <= s_stable_rst;
end rtl;

