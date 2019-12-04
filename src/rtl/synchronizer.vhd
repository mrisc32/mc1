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
-- This is a two-flip-flop synchronization circuit for multi-bit signals.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity synchronizer is
  generic(
    BITS : positive := 32
  );
  port(
    -- Clock signal for the target clock domain.
    i_clk : in std_logic;

    -- Signal from the source clock domain (or an asynchronous signal).
    i_d : in std_logic_vector(BITS-1 downto 0);

    -- Synchronized signal.
    o_q : out std_logic_vector(BITS-1 downto 0)
  );
end synchronizer;

architecture rtl of synchronizer is
  signal s_metastable : std_logic_vector(BITS-1 downto 0);
  signal s_stable : std_logic_vector(BITS-1 downto 0);

  -- Intel/Altera specific constraints.
  attribute ALTERA_ATTRIBUTE : string;
  attribute ALTERA_ATTRIBUTE of rtl : architecture is "-name SDC_STATEMENT ""set_false_path -to [get_registers {*|synchronizer:*|s_metastable*}] """;
  attribute ALTERA_ATTRIBUTE of s_metastable : signal is "-name SYNCHRONIZER_IDENTIFICATION ""FORCED IF ASYNCHRONOUS""";
  attribute PRESERVE : boolean;
  attribute PRESERVE of s_metastable : signal is true;
  attribute PRESERVE of s_stable : signal is true;

  -- Xilinx specific constraints.
  attribute ASYNC_REG : string;
  attribute ASYNC_REG of s_metastable : signal is "TRUE";
  attribute SHREG_EXTRACT : string;
  attribute SHREG_EXTRACT of s_metastable : signal is "NO";
  attribute SHREG_EXTRACT of s_stable : signal is "NO";
begin
  -- Implement two flip-flops in series.
  process(i_clk)
  begin
    if rising_edge(i_clk) then
      s_metastable <= i_d;
      s_stable <= s_metastable;
    end if;
  end process;

  o_q <= s_stable;
end rtl;
