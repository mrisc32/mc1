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
-- This is pseudo-random number generator (PRNG) that is implemented as a Linear Feedback Shift
-- Register (LFSR).
--
-- Since the PRNG is primarily intended to be used for dithering the video signal, we need a fairly
-- long sequence (before repeating the signal) to avoid nasty spatial or temporal patterns. A quick
-- estimation of the required sequence lenght is (for 1080p HD):
--
--    2475000 pixels/frame x 60 FPS x 10 seconds = 1485000000 pixels / 10 s
--
-- This number can be represented with 31 bits. Thus a 32-bit LFSR is sufficient.
----------------------------------------------------------------------------------------------------

entity prng is
  generic(
    START_VALUE : std_logic_vector(32 downto 1) := x"8654af40"
  );
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;
    o_rnd : out std_logic
  );
end prng;

architecture rtl of prng is
  signal s_state : std_logic_vector(32 downto 1);
  signal s_feedback : std_logic;
begin
  -- 32-bit LFSR taps: 32, 22, 2, 1
  s_feedback <= not (s_state(32) xor
                     s_state(22) xor
                     s_state(2) xor
                     s_state(1));

  process(i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_state <= START_VALUE;
    elsif rising_edge(i_clk) then
      s_state <= s_state(31 downto 1) & s_feedback;
    end if;
  end process;

  o_rnd <= s_state(s_state'left);
end rtl;

