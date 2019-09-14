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

----------------------------------------------------------------------------------------------------
-- Video control program processor.
--
-- The VCPP is a high-performance program execution pipeline:
--
--   PC -> IF -> ID/EX -> WR
--
-- The program is restarted on a fixed memory address every time i_restart_frame goes high.
--
-- On every clock cycle a data word is fetched from memory. This word may or may not be fetched
-- from a new address, or from the same address that was previously read, depending on if there was
-- a stall of some sort (either if a WAIT instruction is pausing the execution, or if the memory
-- did not respond with an ACK).
----------------------------------------------------------------------------------------------------

entity vid_vcpp is
  generic(
    X_COORD_BITS : positive;
    Y_COORD_BITS : positive
  );
  port(
    i_rst : in std_logic;
    i_clk : in std_logic;

    i_restart_frame : in std_logic;
    i_raster_x : in std_logic_vector(X_COORD_BITS-1 downto 0);
    i_raster_y : in std_logic_vector(Y_COORD_BITS-1 downto 0);

    o_mem_read_en : out std_logic;
    o_mem_read_addr : out std_logic_vector(23 downto 0);
    i_mem_data : in std_logic_vector(31 downto 0);
    i_mem_ack : in std_logic;

    o_reg_write_enable : out std_logic;
    o_pal_write_enable : out std_logic;
    o_write_addr : out std_logic_vector(7 downto 0);
    o_write_data : out std_logic_vector(31 downto 0)
  );
end vid_vcpp;

architecture rtl of vid_vcpp is
  constant C_ADDR_BITS : positive := 24;
  subtype T_ADDR is std_logic_vector(C_ADDR_BITS-1 downto 0);
  constant C_VCP_START_ADDRESS : T_ADDR := 24x"0";

  subtype T_INSTR is std_logic_vector(1 downto 0);
  subtype T_SUB_INSTR is std_logic_vector(1 downto 0);

  constant C_INSTR_JUMP : T_INSTR := "00";
  constant C_INSTR_WAIT : T_INSTR := "01";
  constant C_INSTR_SETREG : T_INSTR := "10";
  constant C_INSTR_SETPAL : T_INSTR := "11";

  -- JUMP sub instructions.
  constant C_INSTR_NOP : T_SUB_INSTR := "00";
  constant C_INSTR_JMP : T_SUB_INSTR := "01";
  constant C_INSTR_JSR : T_SUB_INSTR := "10";
  constant C_INSTR_RTS : T_SUB_INSTR := "11";

  -- WAIT sub instructions.
  constant C_INSTR_WAITX : T_SUB_INSTR := "00";
  constant C_INSTR_WAITY : T_SUB_INSTR := "01";

  -- PC signals
  signal s_stall_pc : std_logic;
  signal s_pc_read_en : std_logic;
  signal s_pc_read_addr : T_ADDR;
  signal s_pc_prev_read_addr : T_ADDR;
  signal s_pc_prev_read_addr_plus_1 : T_ADDR;
  signal s_pc_expect_ack : std_logic;

  -- IF signals
  signal s_stall_if : std_logic;
  signal s_if_have_data : std_logic;
  signal s_if_data : std_logic_vector(31 downto 0);
  signal s_if_have_cached_data : std_logic;
  signal s_if_latched_data : std_logic_vector(31 downto 0);
  signal s_if_have_latched_data : std_logic;
  signal s_if_is_valid_instr : std_logic;
  signal s_if_pc_plus_1 : T_ADDR;

  -- ID/EX signals
  signal s_id_instr : T_INSTR;
  signal s_id_sub_instr : T_SUB_INSTR;
  signal s_id_is_new_valid_instr : std_logic;

  signal s_id_is_jump_instr : std_logic;
  signal s_id_do_stack_push : std_logic;
  signal s_id_do_stack_pop : std_logic;
  signal s_id_return_addr_from_stack : T_ADDR;
  signal s_id_jump_target : T_ADDR;
  signal s_id_apply_jump_target : std_logic;

  signal s_id_is_waitx_instr : std_logic;
  signal s_id_is_waity_instr : std_logic;
  signal s_id_prev_is_waitx_instr : std_logic;
  signal s_id_prev_is_waity_instr : std_logic;
  signal s_id_xpos_reached : std_logic;
  signal s_id_ypos_reached : std_logic;
  signal s_id_is_waiting : std_logic;

  signal s_id_is_setreg_instr : std_logic;
  signal s_id_reg_addr : std_logic_vector(7 downto 0);
  signal s_id_reg_data : std_logic_vector(31 downto 0);

  signal s_id_is_setpal_instr : std_logic;
  signal s_id_prev_is_setpal_instr : std_logic;
  signal s_id_next_is_pal_data : std_logic;
  signal s_id_is_pal_data : std_logic;
  signal s_id_is_valid_pal_data : std_logic;
  signal s_id_pal_entries_left : std_logic_vector(7 downto 0);
  signal s_id_prev_pal_entries_left : std_logic_vector(7 downto 0);
  signal s_id_prev_pal_entries_left_minus_1 : std_logic_vector(7 downto 0);
  signal s_id_pal_addr : std_logic_vector(7 downto 0);
  signal s_id_prev_pal_addr : std_logic_vector(7 downto 0);
  signal s_id_prev_pal_addr_plus_1 : std_logic_vector(7 downto 0);
  signal s_id_pal_data : std_logic_vector(31 downto 0);

  signal s_id_next_reg_write_enable : std_logic;
  signal s_id_reg_write_enable : std_logic;
  signal s_id_next_pal_write_enable : std_logic;
  signal s_id_pal_write_enable : std_logic;
  signal s_id_next_write_addr : std_logic_vector(7 downto 0);
  signal s_id_write_addr : std_logic_vector(7 downto 0);
  signal s_id_next_write_data : std_logic_vector(31 downto 0);
  signal s_id_write_data : std_logic_vector(31 downto 0);

  function xcoord_to_signed16(x: std_logic_vector) return std_logic_vector is
    variable v_result : std_logic_vector(15 downto 0);
  begin
    v_result(15 downto X_COORD_BITS) := (others => x(X_COORD_BITS-1));
    v_result(X_COORD_BITS-1 downto 0) := x;
    return v_result;
  end;

  function ycoord_to_signed16(y: std_logic_vector) return std_logic_vector is
    variable v_result : std_logic_vector(15 downto 0);
  begin
    v_result(15 downto Y_COORD_BITS) := (others => y(Y_COORD_BITS-1));
    v_result(Y_COORD_BITS-1 downto 0) := y;
    return v_result;
  end;
begin
  -----------------------------------------------------------------------------
  -- PC
  -----------------------------------------------------------------------------

  -- Increment the PC by one.
  s_pc_prev_read_addr_plus_1 <= std_logic_vector(unsigned(s_pc_prev_read_addr) +
                                                 to_unsigned(1, C_ADDR_BITS));

  -- Select which address to read from.
  s_pc_read_addr <= C_VCP_START_ADDRESS when i_restart_frame = '1' else
                    s_id_jump_target when s_id_apply_jump_target = '1' else
                    s_pc_prev_read_addr when s_stall_pc = '1' else
                    s_pc_prev_read_addr_plus_1;

  -- Do we need to send a read request to the memory (i.e. are we requesting a
  -- new word)?
  s_pc_read_en <= '1' when (i_restart_frame = '1' or s_id_apply_jump_target = '1') else
                  '0' when (s_stall_pc = '1' and s_if_have_data = '1') else
                  '1';

  -- PC registers.
  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      -- TODO(m): We should really disable the pipeline altogether until we
      -- get the first i_restart_frame.
      s_pc_prev_read_addr <= C_VCP_START_ADDRESS;
      s_pc_expect_ack <= '0';
    elsif rising_edge(i_clk) then
      s_pc_prev_read_addr <= s_pc_read_addr;
      s_pc_expect_ack <= s_pc_read_en;
    end if;
  end process;

  -- Outputs.
  o_mem_read_en <= s_pc_read_en;
  o_mem_read_addr <= s_pc_read_addr;


  -----------------------------------------------------------------------------
  -- IF
  -----------------------------------------------------------------------------

  -- Do we have the data that we need?
  s_if_have_data <= i_mem_ack when s_pc_expect_ack = '1' else s_if_have_cached_data;

  -- IF registers.
  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      s_if_data <= (others => '0');
      s_if_have_cached_data <= '0';
      s_if_latched_data <= (others => '0');
      s_if_have_latched_data <= '0';
      s_if_is_valid_instr <= '0';
      s_if_pc_plus_1 <= (others => '0');
    elsif rising_edge(i_clk) then
      if s_stall_if = '0' then
        if s_if_have_latched_data = '1' then
          s_if_data <= s_if_latched_data;
        elsif i_mem_ack = '1' then
          s_if_data <= i_mem_data;
        end if;
        s_if_have_cached_data <= s_if_have_data;
        s_if_have_latched_data <= '0';
        s_if_pc_plus_1 <= s_pc_prev_read_addr_plus_1;
      else
        -- During stalls we need to latch read data for later.
        if i_mem_ack = '1' then
          s_if_latched_data <= i_mem_data;
          s_if_have_latched_data <= '1';
        end if;
      end if;
      s_if_is_valid_instr <= s_if_have_data and not (i_restart_frame or s_id_apply_jump_target);
    end if;
  end process;

  -- Stall PC?
  s_stall_pc <= (not s_if_have_data) or s_stall_if;


  -----------------------------------------------------------------------------
  -- ID/EX
  -- TODO(m): If we need to run at higher clock frequencies, split this stage
  -- into two pipeline stages.
  -----------------------------------------------------------------------------

  -- Instantiate the VCPP call stack.
  stack: entity work.vid_vcpp_stack
    port map(
      i_rst => i_rst,
      i_clk => i_clk,
      i_push => s_id_do_stack_push,
      i_pop => s_id_do_stack_pop,
      i_data => s_if_pc_plus_1,
      o_data => s_id_return_addr_from_stack
    );

  -- Decode the instruction.
  s_id_instr <= s_if_data(31 downto 30);
  s_id_sub_instr <= s_if_data(25 downto 24);
  s_id_is_new_valid_instr <= s_if_is_valid_instr and not s_id_is_pal_data;

  -- JUMP: Jump to or return from subroutine.
  s_id_is_jump_instr <= '0' when i_restart_frame = '1' else
                        s_id_is_new_valid_instr when s_id_instr = C_INSTR_JUMP else
                        '0';
  s_id_do_stack_push <= s_id_is_jump_instr when s_id_sub_instr = C_INSTR_JSR else '0';
  s_id_do_stack_pop <= s_id_is_jump_instr when s_id_sub_instr = C_INSTR_RTS else '0';

  -- Determine the next jump target address, and whether or not to perform the jump.
  s_id_jump_target <= s_id_return_addr_from_stack when s_id_do_stack_pop = '1' else s_if_data(23 downto 0);
  s_id_apply_jump_target <= s_id_is_jump_instr when s_id_sub_instr /= C_INSTR_NOP else '0';

  -- WAIT: Should we wait (stall)?
  s_id_is_waitx_instr <= '0' when i_restart_frame = '1' else
                         s_id_prev_is_waitx_instr when s_if_is_valid_instr = '0' else
                         s_id_is_new_valid_instr when s_id_instr = C_INSTR_WAIT and s_id_sub_instr = C_INSTR_WAITX else
                         '0';
  s_id_is_waity_instr <= '0' when i_restart_frame = '1' else
                         s_id_prev_is_waity_instr when s_if_is_valid_instr = '0' else
                         s_id_is_new_valid_instr when s_id_instr = C_INSTR_WAIT and s_id_sub_instr = C_INSTR_WAITY else
                         '0';
  s_id_xpos_reached <= '1' when s_if_data(15 downto 0) = xcoord_to_signed16(i_raster_x) else '0';
  s_id_ypos_reached <= '1' when s_if_data(15 downto 0) = ycoord_to_signed16(i_raster_y) else '0';
  s_id_is_waiting <= (not s_id_xpos_reached) when s_id_is_waitx_instr = '1' else
                     (not s_id_ypos_reached) when s_id_is_waity_instr = '1' else
                     '0';

  -- SETREG: Decode register write operations.
  s_id_is_setreg_instr <= s_id_is_new_valid_instr when s_id_instr = C_INSTR_SETREG else '0';
  s_id_reg_addr <= "00" & s_if_data(29 downto 24);
  s_id_reg_data <= "00000000" & s_if_data(23 downto 0);

  -- SETPAL: Palette state machine.
  s_id_is_setpal_instr <= s_id_is_new_valid_instr when s_id_instr = C_INSTR_SETPAL else '0';
  s_id_next_is_pal_data <= '1' when s_id_is_setpal_instr = '1' else
                           s_id_is_pal_data when s_id_prev_pal_entries_left /= 8x"0" or s_if_is_valid_instr = '0' else
                           '0';
  s_id_is_valid_pal_data <= s_id_is_pal_data and s_if_is_valid_instr;

  s_id_prev_pal_entries_left_minus_1 <= std_logic_vector(unsigned(s_id_prev_pal_entries_left) - to_unsigned(1, 8));
  s_id_pal_entries_left <= s_if_data(7 downto 0) when s_id_is_setpal_instr = '1' else
                           s_id_prev_pal_entries_left_minus_1 when s_id_is_valid_pal_data = '1' else
                           s_id_prev_pal_entries_left;

  s_id_prev_pal_addr_plus_1 <= std_logic_vector(unsigned(s_id_prev_pal_addr) + to_unsigned(1, 8));
  s_id_pal_addr <= s_if_data(15 downto 8) when s_id_is_setpal_instr = '1' else
                   s_id_prev_pal_addr_plus_1 when s_id_is_valid_pal_data = '1' and s_id_prev_is_setpal_instr = '0' else
                   s_id_prev_pal_addr;
  s_id_pal_data <= s_if_data;

  -- Determine the register/palette write operation.
  s_id_next_reg_write_enable <= s_id_is_setreg_instr;
  s_id_next_pal_write_enable <= s_id_is_valid_pal_data;
  s_id_next_write_addr <= s_id_reg_addr when s_id_is_setreg_instr = '1' else
                          s_id_pal_addr;
  s_id_next_write_data <= s_id_reg_data when s_id_is_setreg_instr = '1' else
                          s_id_pal_data;

  -- ID/EX registers.
  process(i_clk, i_rst)
  begin
    if i_rst = '1' then
      s_id_prev_is_waitx_instr <= '0';
      s_id_prev_is_waity_instr <= '0';
      s_id_prev_is_setpal_instr <= '0';
      s_id_is_pal_data <= '0';
      s_id_prev_pal_entries_left <= (others => '0');
      s_id_prev_pal_addr <= (others => '0');
      s_id_reg_write_enable <= '0';
      s_id_pal_write_enable <= '0';
      s_id_write_addr <= (others => '0');
      s_id_write_data <= (others => '0');
    elsif rising_edge(i_clk) then
      s_id_prev_is_waitx_instr <= s_id_is_waitx_instr;
      s_id_prev_is_waity_instr <= s_id_is_waity_instr;
      s_id_prev_is_setpal_instr <= s_id_is_setpal_instr;
      s_id_is_pal_data <= s_id_next_is_pal_data;
      s_id_prev_pal_entries_left <= s_id_pal_entries_left;
      s_id_prev_pal_addr <= s_id_pal_addr;
      s_id_reg_write_enable <= s_id_next_reg_write_enable;
      s_id_pal_write_enable <= s_id_next_pal_write_enable;
      s_id_write_addr <= s_id_next_write_addr;
      s_id_write_data <= s_id_next_write_data;
    end if;
  end process;

  -- Stall IF?
  s_stall_if <= s_id_is_waiting;


  -----------------------------------------------------------------------------
  -- WR - Write data to the video registers or the palette.
  -----------------------------------------------------------------------------

  -- Outputs.
  o_reg_write_enable <= s_id_reg_write_enable;
  o_pal_write_enable <= s_id_pal_write_enable;
  o_write_addr <= s_id_write_addr;
  o_write_data <= s_id_write_data;
end rtl;
