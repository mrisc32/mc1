----------------------------------------------------------------------------------------------------
-- Copyright (c) 2020 Marcus Geelnard
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
use work.vid_types.all;

entity vid_blend is
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;
    i_method : in std_logic_vector(7 downto 0);
    i_color_1 : in std_logic_vector(31 downto 0);
    i_color_2 : in std_logic_vector(31 downto 0);
    o_color : out std_logic_vector(31 downto 0)
  );
end vid_blend;

architecture rtl of vid_blend is
  signal s_b1_blend_1 : unsigned(8 downto 0);
  signal s_b1_blend_2 : unsigned(8 downto 0);
  signal s_b1_r_1 : unsigned(7 downto 0);
  signal s_b1_g_1 : unsigned(7 downto 0);
  signal s_b1_b_1 : unsigned(7 downto 0);
  signal s_b1_r_2 : unsigned(7 downto 0);
  signal s_b1_g_2 : unsigned(7 downto 0);
  signal s_b1_b_2 : unsigned(7 downto 0);

  signal s_b2_r_1 : unsigned(16 downto 0);
  signal s_b2_g_1 : unsigned(16 downto 0);
  signal s_b2_b_1 : unsigned(16 downto 0);
  signal s_b2_r_2 : unsigned(16 downto 0);
  signal s_b2_g_2 : unsigned(16 downto 0);
  signal s_b2_b_2 : unsigned(16 downto 0);
begin
  --------------------------------------------------------------------------------------------------
  -- B1 - Prepare the blend factors.
  --------------------------------------------------------------------------------------------------

  process(i_clk, i_rst)
    subtype T_BLEND_SEL is unsigned(2 downto 0);
    constant C_BLEND_ONE              : T_BLEND_SEL := "000";
    constant C_BLEND_MINUS_ONE        : T_BLEND_SEL := "001";
    constant C_BLEND_ALPHA1           : T_BLEND_SEL := "010";
    constant C_BLEND_ALPHA2           : T_BLEND_SEL := "011";
    constant C_BLEND_ONE_MINUS_ALPHA1 : T_BLEND_SEL := "100";
    constant C_BLEND_ONE_MINUS_ALPHA2 : T_BLEND_SEL := "101";

    variable v_alpha1_raw : unsigned(8 downto 0);
    variable v_alpha2_raw : unsigned(8 downto 0);
    variable v_alpha1 : unsigned(8 downto 0);
    variable v_alpha2 : unsigned(8 downto 0);
    variable v_alpha1_compl : unsigned(8 downto 0);
    variable v_alpha2_compl : unsigned(8 downto 0);

    variable v_sel1 : T_BLEND_SEL;
    variable v_sel2 : T_BLEND_SEL;
    variable v_blend1 : unsigned(8 downto 0);
    variable v_blend2 : unsigned(8 downto 0);
  begin
    if i_rst = '1' then
      s_b1_blend_1 <= (others => '0');
      s_b1_blend_2 <= (others => '0');
      s_b1_r_1 <= (others => '0');
      s_b1_g_1 <= (others => '0');
      s_b1_b_1 <= (others => '0');
      s_b1_r_2 <= (others => '0');
      s_b1_g_2 <= (others => '0');
      s_b1_b_2 <= (others => '0');
    elsif rising_edge(i_clk) then
      -- Extract the raw alpha channel from color 1 & color 2.
      v_alpha1_raw := unsigned('0' & i_color_1(31 downto 24));
      v_alpha2_raw := unsigned('0' & i_color_2(31 downto 24));

      -- Convert the alpha to the range [1, 256], and its complement in the range [1, 256].
      v_alpha1 := v_alpha1_raw + 1;
      v_alpha1_compl := 256 - v_alpha1_raw;
      v_alpha2 := v_alpha2_raw + 1;
      v_alpha2_compl := 256 - v_alpha2_raw;

      -- Select the blend factor for color 1.
      v_sel1 := unsigned(i_method(2 downto 0));
      case v_sel1 is
        when C_BLEND_ONE =>
          v_blend1 := 9x"100";
        when C_BLEND_MINUS_ONE =>
          v_blend1 := 9x"100";  -- TODO(m): Add support for negative scale factors!
        when C_BLEND_ALPHA1 =>
          v_blend1 := v_alpha1;
        when C_BLEND_ALPHA2 =>
          v_blend1 := v_alpha2;
        when C_BLEND_ONE_MINUS_ALPHA1 =>
          v_blend1 := v_alpha1_compl;
        when C_BLEND_ONE_MINUS_ALPHA2 =>
          v_blend1 := v_alpha2_compl;
        when others =>
          v_blend1 := 9x"100";
      end case;

      -- Select the blend factor for color 2.
      v_sel2 := unsigned(i_method(6 downto 4));
      case v_sel2 is
        when C_BLEND_ONE =>
          v_blend2 := 9x"100";
        when C_BLEND_MINUS_ONE =>
          v_blend2 := 9x"100";  -- TODO(m): Add support for negative scale factors!
        when C_BLEND_ALPHA1 =>
          v_blend2 := v_alpha1;
        when C_BLEND_ALPHA2 =>
          v_blend2 := v_alpha2;
        when C_BLEND_ONE_MINUS_ALPHA1 =>
          v_blend2 := v_alpha1_compl;
        when C_BLEND_ONE_MINUS_ALPHA2 =>
          v_blend2 := v_alpha2_compl;
        when others =>
          v_blend2 := 9x"100";
      end case;

      s_b1_blend_1 <= v_blend1;
      s_b1_blend_2 <= v_blend2;

      -- Extract the color components.
      s_b1_r_1 <= unsigned(i_color_1(7 downto 0));
      s_b1_g_1 <= unsigned(i_color_1(15 downto 8));
      s_b1_b_1 <= unsigned(i_color_1(23 downto 16));
      s_b1_r_2 <= unsigned(i_color_2(7 downto 0));
      s_b1_g_2 <= unsigned(i_color_2(15 downto 8));
      s_b1_b_2 <= unsigned(i_color_2(23 downto 16));
    end if;
  end process;


  --------------------------------------------------------------------------------------------------
  -- B2 - Scale all channels using the given blend factors.
  --------------------------------------------------------------------------------------------------

  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      s_b2_r_1 <= (others => '0');
      s_b2_g_1 <= (others => '0');
      s_b2_b_1 <= (others => '0');
      s_b2_r_2 <= (others => '0');
      s_b2_g_2 <= (others => '0');
      s_b2_b_2 <= (others => '0');
    elsif rising_edge(i_clk) then
      -- Scale color 1.
      s_b2_r_1 <= s_b1_r_1 * s_b1_blend_1;
      s_b2_g_1 <= s_b1_g_1 * s_b1_blend_1;
      s_b2_b_1 <= s_b1_b_1 * s_b1_blend_1;

      -- Scale color 2.
      s_b2_r_2 <= s_b1_r_2 * s_b1_blend_2;
      s_b2_g_2 <= s_b1_g_2 * s_b1_blend_2;
      s_b2_b_2 <= s_b1_b_2 * s_b1_blend_2;
    end if;
  end process;


  --------------------------------------------------------------------------------------------------
  -- B3 - Blend all channels to the final color.
  --------------------------------------------------------------------------------------------------

  process(i_clk, i_rst)
    variable v_r : std_logic_vector(16 downto 0);
    variable v_g : std_logic_vector(16 downto 0);
    variable v_b : std_logic_vector(16 downto 0);

    variable v_r_clamped : std_logic_vector(7 downto 0);
    variable v_g_clamped : std_logic_vector(7 downto 0);
    variable v_b_clamped : std_logic_vector(7 downto 0);
  begin
    if i_rst = '1' then
      o_color <= (others => '0');
    elsif rising_edge(i_clk) then
      -- Blend (add the scaled components).
      v_r := std_logic_vector(s_b2_r_1 + s_b2_r_2);
      v_g := std_logic_vector(s_b2_g_1 + s_b2_g_2);
      v_b := std_logic_vector(s_b2_b_1 + s_b2_b_2);

      -- Clamp the R, G and B channels to the range [0, 255].
      if (v_r(16) = '0') then
        v_r_clamped := v_r(15 downto 8);
      else
        v_r_clamped := x"ff";
      end if;
      if (v_g(16) = '0') then
        v_g_clamped := v_g(15 downto 8);
      else
        v_g_clamped := x"ff";
      end if;
      if (v_b(16) = '0') then
        v_b_clamped := v_b(15 downto 8);
      else
        v_b_clamped := x"ff";
      end if;

      -- Compose the final color (ABGR32).
      o_color <= x"ff" & v_b_clamped & v_g_clamped & v_r_clamped;
    end if;
  end process;

end rtl;
