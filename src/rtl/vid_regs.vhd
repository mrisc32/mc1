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
use work.vid_types.all;

----------------------------------------------------------------------------------------------------
-- Video control registers.
----------------------------------------------------------------------------------------------------

entity vid_regs is
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;

    i_write_enable : in std_logic;
    i_write_addr : in std_logic_vector(2 downto 0);
    i_write_data : in std_logic_vector(23 downto 0);

    o_regs : out T_VID_REGS
  );
end vid_regs;

architecture rtl of vid_regs is
  constant C_DEFAULT_ADDR : std_logic_vector(23 downto 0) := x"000000";
  constant C_DEFAULT_XOFFS : std_logic_vector(23 downto 0) := x"000000";
  constant C_DEFAULT_XINCR : std_logic_vector(23 downto 0) := x"004000";
  constant C_DEFAULT_HSTRT : std_logic_vector(23 downto 0) := x"000000";
  constant C_DEFAULT_HSTOP : std_logic_vector(23 downto 0) := x"000000";
  constant C_DEFAULT_CMODE : std_logic_vector(23 downto 0) := x"000002";

  signal s_reg_ADDR : std_logic_vector(23 downto 0);
  signal s_reg_XOFFS : std_logic_vector(23 downto 0);
  signal s_reg_XINCR : std_logic_vector(23 downto 0);
  signal s_reg_HSTRT : std_logic_vector(23 downto 0);
  signal s_reg_HSTOP : std_logic_vector(23 downto 0);
  signal s_reg_CMODE : std_logic_vector(23 downto 0);

  signal s_next_ADDR : std_logic_vector(23 downto 0);
  signal s_next_XOFFS : std_logic_vector(23 downto 0);
  signal s_next_XINCR : std_logic_vector(23 downto 0);
  signal s_next_HSTRT : std_logic_vector(23 downto 0);
  signal s_next_HSTOP : std_logic_vector(23 downto 0);
  signal s_next_CMODE : std_logic_vector(23 downto 0);
begin
  -- Write logic.
  s_next_ADDR <= i_write_data when i_write_enable = '1' and i_write_addr = "000" else
                 s_reg_ADDR;
  s_next_XOFFS <= i_write_data when i_write_enable = '1' and i_write_addr = "001" else
                  s_reg_XOFFS;
  s_next_XINCR <= i_write_data when i_write_enable = '1' and i_write_addr = "010" else
                  s_reg_XINCR;
  s_next_HSTRT <= i_write_data when i_write_enable = '1' and i_write_addr = "011" else
                  s_reg_HSTRT;
  s_next_HSTOP <= i_write_data when i_write_enable = '1' and i_write_addr = "100" else
                  s_reg_HSTOP;
  s_next_CMODE <= i_write_data when i_write_enable = '1' and i_write_addr = "101" else
                  s_reg_CMODE;

  -- Clocked registers.
  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      s_reg_ADDR <= C_DEFAULT_ADDR;
      s_reg_XOFFS <= C_DEFAULT_XOFFS;
      s_reg_XINCR <= C_DEFAULT_XINCR;
      s_reg_HSTRT <= C_DEFAULT_HSTRT;
      s_reg_HSTOP <= C_DEFAULT_HSTOP;
      s_reg_CMODE <= C_DEFAULT_CMODE;
    elsif rising_edge(i_clk) then
      s_reg_ADDR <= s_next_ADDR;
      s_reg_XOFFS <= s_next_XOFFS;
      s_reg_XINCR <= s_next_XINCR;
      s_reg_HSTRT <= s_next_HSTRT;
      s_reg_HSTOP <= s_next_HSTOP;
      s_reg_CMODE <= s_next_CMODE;
    end if;
  end process;

  -- Outputs.
  o_regs.ADDR <= s_reg_ADDR;
  o_regs.XOFFS <= s_reg_XOFFS;
  o_regs.XINCR <= s_reg_XINCR;
  o_regs.HSTRT <= s_reg_HSTRT;
  o_regs.HSTOP <= s_reg_HSTOP;
  o_regs.CMODE <= s_reg_CMODE;
end rtl;
