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
-- This is a two-flip-flop synchronization circuit for single bit signals.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity synchronizer1 is
  port(
    -- Clock signal for this clock domain.
    i_clk : in std_logic;

    -- Signal from another clock domain, or an asynchronous signal.
    i_d : in std_logic;

    -- Synchronized signal.
    o_q : out std_logic
  );
end synchronizer1;

architecture rtl of synchronizer1 is
  signal s_q_1 : std_logic;
begin
  -- Implement two flip-flops in series.
  process(i_clk)
  begin
    if rising_edge(i_clk) then
      s_q_1 <= i_d;
      o_q <= s_q_1;
    end if;
  end process;
end rtl;


----------------------------------------------------------------------------------------------------
-- This is a two-flip-flop synchronization circuit for multi-bit signals.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity synchronizer is
  generic(
    BITS : positive := 32
  );
  port(
    -- Clock signal for this clock domain.
    i_clk : in std_logic;

    -- Signal from another clock domain, or an asynchronous signal.
    i_d : in std_logic_vector(BITS-1 downto 0);

    -- Synchronized signal.
    o_q : out std_logic_vector(BITS-1 downto 0)
  );
end synchronizer;

architecture rtl of synchronizer is
  signal s_q_1 : std_logic_vector(BITS-1 downto 0);
begin
  -- Implement two flip-flops in series.
  process(i_clk)
  begin
    if rising_edge(i_clk) then
      s_q_1 <= i_d;
      o_q <= s_q_1;
    end if;
  end process;
end rtl;
