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
-- This is a single-ported RAM module with the following properties:
--   * Wishbone B4 pipelined interface.
--   * Configurable size (2^N words).
--   * 32-bit data width.
--   * Byte enable / select for write operations.
--   * Single cycle read/write operation.
--   * Synthesizes to BRAM (tested on Intel Arria II, Cyclone IV, Cyclone V, Cyclone 10, MAX 10).
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ram is
  generic(
    ADR_BITS : positive := 10  -- 2**10 = 1024 words
  );
  port(
    -- Reset signal.
    i_rst : in std_logic;

    -- Wishbone memory interface (b4 pipelined slave).
    -- See: https://cdn.opencores.org/downloads/wbspec_b4.pdf
    i_wb_clk : in std_logic;
    i_wb_cyc : in std_logic;
    i_wb_stb : in std_logic;
    i_wb_adr : in std_logic_vector(ADR_BITS-1 downto 0);
    i_wb_dat : in std_logic_vector(31 downto 0);
    i_wb_we : in std_logic;
    i_wb_sel : in std_logic_vector(32/8-1 downto 0);
    o_wb_dat : out std_logic_vector(31 downto 0);
    o_wb_ack : out std_logic;
    o_wb_stall : out std_logic;

    -- Read-only second port to the RAM.
    i_read_clk : in std_logic;
    i_read_adr : in std_logic_vector(ADR_BITS-1 downto 0);
    o_read_dat : out std_logic_vector(31 downto 0)
  );
end ram;

architecture rtl of ram is
  constant C_NUM_WORDS : positive := 2**ADR_BITS;

  type T_BYTE_ARRAY is array (0 to C_NUM_WORDS-1) of std_logic_vector(7 downto 0);
  signal s_byte_array_0 : T_BYTE_ARRAY;
  signal s_byte_array_1 : T_BYTE_ARRAY;
  signal s_byte_array_2 : T_BYTE_ARRAY;
  signal s_byte_array_3 : T_BYTE_ARRAY;
begin
  process(i_wb_clk)
    variable v_adr : integer range 0 to C_NUM_WORDS-1;
    variable v_is_valid_request : std_logic;
  begin
    if rising_edge(i_wb_clk) then
      -- Is this a valid request for this Wishbone slave?
      v_is_valid_request := (not i_rst) and i_wb_cyc and i_wb_stb;

      -- Get the address.
      v_adr := to_integer(unsigned(i_wb_adr));

      -- Write?
      if v_is_valid_request = '1' and i_wb_we = '1' then
        if i_wb_sel(0) = '1' then
          s_byte_array_0(v_adr) <= i_wb_dat(7 downto 0);
        end if;
        if i_wb_sel(1) = '1' then
          s_byte_array_1(v_adr) <= i_wb_dat(15 downto 8);
        end if;
        if i_wb_sel(2) = '1' then
          s_byte_array_2(v_adr) <= i_wb_dat(23 downto 16);
        end if;
        if i_wb_sel(3) = '1' then
          s_byte_array_3(v_adr) <= i_wb_dat(31 downto 24);
        end if;
      end if;

      -- We always read.
      o_wb_dat(7 downto 0) <= s_byte_array_0(v_adr);
      o_wb_dat(15 downto 8) <= s_byte_array_1(v_adr);
      o_wb_dat(23 downto 16) <= s_byte_array_2(v_adr);
      o_wb_dat(31 downto 24) <= s_byte_array_3(v_adr);

      -- Ack that we have dealt with the request.
      o_wb_ack <= v_is_valid_request;
    end if;
  end process;

  -- We never stall - we're that fast ;-)
  o_wb_stall <= '0';

  -- TODO(m): Implement the second read port.
  o_read_dat <= (others => '0');
end rtl;
