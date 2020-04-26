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

library vunit_lib;
context vunit_lib.vunit_context;

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.textio.all;

library mrisc32;
use mrisc32.debug.all;

use work.vid_types.all;

entity mc1_tb is
  generic (runner_cfg : string);
end entity;

architecture tb of mc1_tb is
  -- Number of frames to simulate.
  constant C_TEST_FRAMES : integer := 2;

  -- (640 + hblank) x (480 + vblank) = 420000 cycles per frame
  -- (800 + hblank) x (600 + vblank) = 663168 cycles per frame
  -- (1280 + hblank) x (720 + vblank) = 1237500 cycles per frame
  -- (1920 + hblank) x (1080 + vblank) = 2475000 cycles per frame
  constant C_TEST_CYCLES : integer := 2475000 * C_TEST_FRAMES;

  --  25.175 MHz -> 19.8609732 ns
  --  40.000 MHz -> 12.5 ns
  --  74.375 MHz -> 6.72268908 ns
  -- 148.500 MHz -> 3.36700337 ns
  constant C_CLK_HALF_PERIOD : time := 3.36700337 ns;

  signal s_rst : std_logic;
  signal s_clk : std_logic;
  signal s_r : std_logic_vector(3 downto 0);
  signal s_g : std_logic_vector(3 downto 0);
  signal s_b : std_logic_vector(3 downto 0);
  signal s_hsync : std_logic;
  signal s_vsync : std_logic;

  -- Debug trace interface.
  signal s_debug_trace : T_DEBUG_TRACE;
begin
  -- Instantiate the MC1 machine.
  mc1_1: entity work.mc1
    generic map (
      COLOR_BITS => s_r'length,
      LOG2_VRAM_SIZE => 15,          -- 4*2^15 = 128 KiB
      VIDEO_CONFIG => C_1920_1080
    )
    port map (
      -- Control signals.
      i_cpu_rst => s_rst,
      i_cpu_clk => s_clk,

      -- VGA interface.
      i_vga_rst => s_rst,
      i_vga_clk => s_clk,
      o_vga_r => s_r,
      o_vga_g => s_g,
      o_vga_b => s_b,
      o_vga_hs => s_hsync,
      o_vga_vs => s_vsync,

      -- I/O interfaces.
      i_io_switches => 32x"4",
      i_io_buttons => (others => '0'),

      -- Debug trace interface.
      o_debug_trace => s_debug_trace
    );

  main : process
    -- File I/O.
    type T_CHAR_FILE is file of character;
    file f_char_file : T_CHAR_FILE;
    file f_trace_file : T_CHAR_FILE;

    -- Helper function for writing one word to a binary file.
    procedure write_word(file f : T_CHAR_FILE; word : std_logic_vector(31 downto 0)) is
      variable v_char : character;
      variable v_byte : std_logic_vector(7 downto 0);
    begin
      for i in 0 to 3 loop
        v_byte := word(((i+1)*8)-1 downto i*8);
        v_char := character'val(to_integer(unsigned(v_byte)));
        write(f, v_char);
      end loop;
    end procedure;

    -- Helper function for writing a debug trace record to a binary file.
    procedure write_trace(file f : T_CHAR_FILE; trace : T_DEBUG_TRACE) is
      variable v_flags : std_logic_vector(31 downto 0);
    begin
      v_flags := (0 => trace.valid,
                  1 => trace.src_a_valid,
                  2 => trace.src_b_valid,
                  3 => trace.src_c_valid,
                  others => '0');
      write_word(f, v_flags);
      write_word(f, trace.pc);
      if trace.src_a_valid then
        write_word(f, trace.src_a);
      else
        write_word(f, (others => '0'));
      end if;
      if trace.src_b_valid then
        write_word(f, trace.src_b);
      else
        write_word(f, (others => '0'));
      end if;
      if trace.src_c_valid then
        write_word(f, trace.src_c);
      else
        write_word(f, (others => '0'));
      end if;
    end procedure;

    variable v_rgb_word : std_logic_vector(31 downto 0);
  begin
    test_runner_setup(runner, runner_cfg);

    -- Continue running even if we have failures (for easier debugging).
    set_stop_level(failure);

    -- Open the debug trace file.
    if C_DEBUG_ENABLE_TRACE then
      file_open(f_trace_file, "vunit_out/mc1_tb_trace.bin", WRITE_MODE);
    end if;

    -- Reset the MC1.
    s_rst <= '1';
    s_clk <= '0';
    wait for C_CLK_HALF_PERIOD;
    s_clk <= '1';
    wait for C_CLK_HALF_PERIOD;
    s_rst <= '0';
    s_clk <= '0';
    wait for C_CLK_HALF_PERIOD;

    -- Run a lot of cycles...
    file_open(f_char_file, "vunit_out/mc1_tb_output.data", WRITE_MODE);
    for i in 0 to C_TEST_CYCLES-1 loop
      -- Construct a word from the generated RGB output.
      -- We inject hsync and vsync into the color channels for visualization.
      v_rgb_word(31 downto 24) := 8x"ff";
      v_rgb_word(23 downto 16) := s_b & s_b(3 downto 0);
      if s_vsync = '1' then
        v_rgb_word(15 downto 8) := 8x"ff";
      else
        v_rgb_word(15 downto 8) := s_g & s_g(3 downto 0);
      end if;
      if s_hsync = '1' then
        v_rgb_word(7 downto 0) := 8x"ff";
      else
        v_rgb_word(7 downto 0) := s_r & s_r(3 downto 0);
      end if;

      -- Write the word to the output file.
      write_word(f_char_file, v_rgb_word);

      -- Write a recrod to the debug trace file.
      -- Note: We skip the first few cycles until we are properly reset.
      if C_DEBUG_ENABLE_TRACE and i >= 4 then
        write_trace(f_trace_file, s_debug_trace);
      end if;

      -- Tick the clock.
      s_clk <= '1';
      wait for C_CLK_HALF_PERIOD;
      s_clk <= '0';
      wait for C_CLK_HALF_PERIOD;
    end loop;
    file_close(f_char_file);

    -- Close the debug trace file.
    if C_DEBUG_ENABLE_TRACE then
      file_close(f_trace_file);
    end if;

    test_runner_cleanup(runner);
  end process;
end architecture;
