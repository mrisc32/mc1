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
-- This is a MUX that directs Wishbone requests to different interfaces depending on the address.
--
-- The two most significant bits of the address dictates which interface will be addressed:
--  - 00000000 - 3fffffff  Interface 0 (1 GiB, e.g. ROM)
--  - 40000000 - 7fffffff  Interface 1 (1 GiB, e.g. internal shared VRAM)
--  - 80000000 - bfffffff  Interface 2 (1 GiB, e.g. external RAM)
--  - c0000000 - ffffffff  Interface 3 (1 GiB, e.g. memory mapped I/O)
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity memory_mux is
  port(
    -- Control signals.
    i_rst : in std_logic;
    i_clk : in std_logic;

    -- Wishbone master interface.
    i_wb_cyc : in std_logic;
    i_wb_stb : in std_logic;
    i_wb_adr : in std_logic_vector(31 downto 2);
    i_wb_dat : in std_logic_vector(31 downto 0);
    i_wb_we : in std_logic;
    i_wb_sel : in std_logic_vector(32/8-1 downto 0);
    o_wb_dat : out std_logic_vector(31 downto 0);
    o_wb_ack : out std_logic;
    o_wb_stall : out std_logic;
    o_wb_err : out std_logic;

    -- Wishbone slave interface 0.
    o_wb_cyc_0 : out std_logic;
    o_wb_stb_0 : out std_logic;
    o_wb_adr_0 : out std_logic_vector(31 downto 2);
    o_wb_dat_0 : out std_logic_vector(31 downto 0);
    o_wb_we_0 : out std_logic;
    o_wb_sel_0 : out std_logic_vector(32/8-1 downto 0);
    i_wb_dat_0 : in std_logic_vector(31 downto 0);
    i_wb_ack_0 : in std_logic;
    i_wb_stall_0 : in std_logic;
    i_wb_err_0 : in std_logic;

    -- Wishbone slave interface 1.
    o_wb_cyc_1 : out std_logic;
    o_wb_stb_1 : out std_logic;
    o_wb_adr_1 : out std_logic_vector(31 downto 2);
    o_wb_dat_1 : out std_logic_vector(31 downto 0);
    o_wb_we_1 : out std_logic;
    o_wb_sel_1 : out std_logic_vector(32/8-1 downto 0);
    i_wb_dat_1 : in std_logic_vector(31 downto 0);
    i_wb_ack_1 : in std_logic;
    i_wb_stall_1 : in std_logic;
    i_wb_err_1 : in std_logic;

    -- Wishbone slave interface 2.
    o_wb_cyc_2 : out std_logic;
    o_wb_stb_2 : out std_logic;
    o_wb_adr_2 : out std_logic_vector(31 downto 2);
    o_wb_dat_2 : out std_logic_vector(31 downto 0);
    o_wb_we_2 : out std_logic;
    o_wb_sel_2 : out std_logic_vector(32/8-1 downto 0);
    i_wb_dat_2 : in std_logic_vector(31 downto 0);
    i_wb_ack_2 : in std_logic;
    i_wb_stall_2 : in std_logic;
    i_wb_err_2 : in std_logic;

    -- Wishbone slave interface 3.
    o_wb_cyc_3 : out std_logic;
    o_wb_stb_3 : out std_logic;
    o_wb_adr_3 : out std_logic_vector(31 downto 2);
    o_wb_dat_3 : out std_logic_vector(31 downto 0);
    o_wb_we_3 : out std_logic;
    o_wb_sel_3 : out std_logic_vector(32/8-1 downto 0);
    i_wb_dat_3 : in std_logic_vector(31 downto 0);
    i_wb_ack_3 : in std_logic;
    i_wb_stall_3 : in std_logic;
    i_wb_err_3 : in std_logic
  );
end memory_mux;

architecture rtl of memory_mux is
  signal s_stb_0 : std_logic;
  signal s_stb_1 : std_logic;
  signal s_stb_2 : std_logic;
  signal s_stb_3 : std_logic;

  signal s_result_port : std_logic_vector(1 downto 0);
  signal s_next_result_port : std_logic_vector(1 downto 0);
begin
  -- Which interface is activated?
  s_stb_0 <= i_wb_stb when i_wb_adr(31 downto 30) = "00" else '0';
  s_stb_1 <= i_wb_stb when i_wb_adr(31 downto 30) = "01" else '0';
  s_stb_2 <= i_wb_stb when i_wb_adr(31 downto 30) = "10" else '0';
  s_stb_3 <= i_wb_stb when i_wb_adr(31 downto 30) = "11" else '0';

  -- Select which interface to receive data from, based on the STB
  -- signals of the previous cycle.
  s_next_result_port <= "00" when s_stb_0 = '1' else
                        "01" when s_stb_1 = '1' else
                        "10" when s_stb_2 = '1' else
                        "11" when s_stb_3 = '1' else
                        s_result_port;
  process(i_rst, i_clk)
  begin
    if i_rst = '1' then
      s_result_port <= (others => '0');
    elsif rising_edge(i_clk) then
      s_result_port <= s_next_result_port;
    end if;
  end process;

  -- Generate control signals for the slaves.
  o_wb_cyc_0 <= i_wb_cyc;
  o_wb_stb_0 <= s_stb_0;
  o_wb_adr_0 <= i_wb_adr;
  o_wb_dat_0 <= i_wb_dat;
  o_wb_we_0 <= i_wb_we;
  o_wb_sel_0 <= i_wb_sel;

  o_wb_cyc_1 <= i_wb_cyc;
  o_wb_stb_1 <= s_stb_1;
  o_wb_adr_1 <= i_wb_adr;
  o_wb_dat_1 <= i_wb_dat;
  o_wb_we_1 <= i_wb_we;
  o_wb_sel_1 <= i_wb_sel;

  o_wb_cyc_2 <= i_wb_cyc;
  o_wb_stb_2 <= s_stb_2;
  o_wb_adr_2 <= i_wb_adr;
  o_wb_dat_2 <= i_wb_dat;
  o_wb_we_2 <= i_wb_we;
  o_wb_sel_2 <= i_wb_sel;

  o_wb_cyc_3 <= i_wb_cyc;
  o_wb_stb_3 <= s_stb_3;
  o_wb_adr_3 <= i_wb_adr;
  o_wb_dat_3 <= i_wb_dat;
  o_wb_we_3 <= i_wb_we;
  o_wb_sel_3 <= i_wb_sel;

  -- We assume that all slaves are well behaving, so we just OR together
  -- their ACK, STALL and ERR signals.
  o_wb_ack <= i_wb_ack_0 or i_wb_ack_1 or i_wb_ack_2 or i_wb_ack_3;
  o_wb_stall <= i_wb_stall_0 or i_wb_stall_1 or i_wb_stall_2 or i_wb_stall_3;
  o_wb_err <= i_wb_err_0 or i_wb_err_1 or i_wb_err_2 or i_wb_err_3;

  -- Select the data result.
  DataMux: with s_result_port select
    o_wb_dat <=
      i_wb_dat_0 when "00",
      i_wb_dat_1 when "01",
      i_wb_dat_2 when "10",
      i_wb_dat_3 when "11",
      (others => '-') when others;
end rtl;
