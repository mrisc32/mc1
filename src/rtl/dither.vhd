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

entity dither is
  generic(
    BITS : positive
  );
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;
    i_method : in std_logic_vector(1 downto 0);
    i_r : in std_logic_vector(7 downto 0);
    i_g : in std_logic_vector(7 downto 0);
    i_b : in std_logic_vector(7 downto 0);
    o_r : out std_logic_vector(BITS-1 downto 0);
    o_g : out std_logic_vector(BITS-1 downto 0);
    o_b : out std_logic_vector(BITS-1 downto 0)
  );
end dither;

architecture rtl of dither is
  constant C_DITHER_BITS : positive := 8 - BITS;

  constant C_METHOD_NONE : std_logic_vector(1 downto 0) := "00";
  constant C_METHOD_WHITE : std_logic_vector(1 downto 0) := "01";

  signal s_rnd : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_rnd_r : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_rnd_g : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_rnd_b : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_next_dither_r : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_next_dither_g : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_next_dither_b : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_dither_r : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_dither_g : std_logic_vector(C_DITHER_BITS-1 downto 0);
  signal s_dither_b : std_logic_vector(C_DITHER_BITS-1 downto 0);

  signal s_dither_r_ext : std_logic_vector(9 downto 0);
  signal s_dither_g_ext : std_logic_vector(9 downto 0);
  signal s_dither_b_ext : std_logic_vector(9 downto 0);
  signal s_next_r_unclamped : std_logic_vector(9 downto 0);
  signal s_next_g_unclamped : std_logic_vector(9 downto 0);
  signal s_next_b_unclamped : std_logic_vector(9 downto 0);
  signal s_r_unclamped : std_logic_vector(9 downto 0);
  signal s_g_unclamped : std_logic_vector(9 downto 0);
  signal s_b_unclamped : std_logic_vector(9 downto 0);

  signal s_next_r : std_logic_vector(BITS-1 downto 0);
  signal s_next_g : std_logic_vector(BITS-1 downto 0);
  signal s_next_b : std_logic_vector(BITS-1 downto 0);

  function prng_start_value(k : integer) return std_logic_vector is
    variable v_bits : std_logic_vector(31 downto 0);
  begin
    v_bits := std_logic_vector(to_unsigned(19088743 + 267242409 * k, 32));
    return v_bits(15 downto 0) & v_bits(31 downto 16);
  end function;

  function clamp_and_trunc(x : std_logic_vector) return std_logic_vector is
  begin
    if x(9) = '1' then
      -- Underflow -> 0
      return std_logic_vector(to_signed(0, BITS));
    elsif x(8) = '1' then
      -- Overflow -> "11...11"
      return std_logic_vector(to_signed(-1, BITS));
    else
      return x(7 downto 8-BITS);
    end if;
  end function;

begin
  --------------------------------------------------------------------------------------------------
  -- Stage 1: Generate dithering noise.
  --------------------------------------------------------------------------------------------------

  -- Use N random number generators to form an N-bit random number.
  -- TODO(m): Investigate if we can extract N bits from the PRNG state instead in order to use less
  -- hardware resources. The risk is that there will be a strong correlation between N consecutive
  -- samples.
  PRNGGen: for k in 0 to C_DITHER_BITS-1 generate
  begin
    prng1: entity work.prng
      generic map(
        START_VALUE => prng_start_value(k)
      )
      port map (
        i_rst => i_rst,
        i_clk => i_clk,
        o_rnd => s_rnd(k)
      );
  end generate;

  -- Construct different random numbers for R, G and B by permuting the bits.
  RGBRndGen: for k in 0 to C_DITHER_BITS-1 generate
  begin
    s_rnd_r(k) <= s_rnd(k);
    s_rnd_g(k) <= s_rnd(C_DITHER_BITS-1 - k);
    s_rnd_b(k) <= s_rnd(k) xor s_rnd(C_DITHER_BITS-1 - k);  -- Can we avoid the XOR?
  end generate;

  -- Select dithering type.
  DitherMuxR: with i_method select
    s_next_dither_r <= s_rnd_r         when C_METHOD_WHITE,
                       (others => '0') when others;

  DitherMuxG: with i_method select
    s_next_dither_g <= s_rnd_g         when C_METHOD_WHITE,
                       (others => '0') when others;

  DitherMuxB: with i_method select
    s_next_dither_b <= s_rnd_b         when C_METHOD_WHITE,
                       (others => '0') when others;

  process(i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_dither_r <= (others => '0');
      s_dither_g <= (others => '0');
      s_dither_b <= (others => '0');
    elsif rising_edge(i_clk) then
      s_dither_r <= s_next_dither_r;
      s_dither_g <= s_next_dither_g;
      s_dither_b <= s_next_dither_b;
    end if;
  end process;


  --------------------------------------------------------------------------------------------------
  -- Stage 2: Apply dithering.
  --------------------------------------------------------------------------------------------------

  -- Sign extend dithering data.
  s_dither_r_ext(C_DITHER_BITS-1 downto 0) <= s_dither_r;
  s_dither_r_ext(9 downto C_DITHER_BITS) <= (others => s_dither_r(C_DITHER_BITS-1));
  s_dither_g_ext(C_DITHER_BITS-1 downto 0) <= s_dither_g;
  s_dither_g_ext(9 downto C_DITHER_BITS) <= (others => s_dither_g(C_DITHER_BITS-1));
  s_dither_b_ext(C_DITHER_BITS-1 downto 0) <= s_dither_b;
  s_dither_b_ext(9 downto C_DITHER_BITS) <= (others => s_dither_b(C_DITHER_BITS-1));

  -- Perform dithering.
  s_next_r_unclamped <= std_logic_vector(signed("00" & i_r) + signed(s_dither_r_ext));
  s_next_g_unclamped <= std_logic_vector(signed("00" & i_g) + signed(s_dither_g_ext));
  s_next_b_unclamped <= std_logic_vector(signed("00" & i_b) + signed(s_dither_b_ext));

  process(i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_r_unclamped <= (others => '0');
      s_g_unclamped <= (others => '0');
      s_b_unclamped <= (others => '0');
    elsif rising_edge(i_clk) then
      s_r_unclamped <= s_next_r_unclamped;
      s_g_unclamped <= s_next_g_unclamped;
      s_b_unclamped <= s_next_b_unclamped;
    end if;
  end process;


  --------------------------------------------------------------------------------------------------
  -- Stage 3: Clamp and truncate.
  --------------------------------------------------------------------------------------------------

  -- Form the final result via clamping and truncation.
  s_next_r <= clamp_and_trunc(s_r_unclamped);
  s_next_g <= clamp_and_trunc(s_g_unclamped);
  s_next_b <= clamp_and_trunc(s_b_unclamped);

  process(i_rst, i_clk)
  begin
    if i_rst = '1' then
      o_r <= (others => '0');
      o_g <= (others => '0');
      o_b <= (others => '0');
    elsif rising_edge(i_clk) then
      o_r <= s_next_r;
      o_g <= s_next_g;
      o_b <= s_next_b;
    end if;
  end process;
end rtl;

