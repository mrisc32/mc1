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
-- Internal memory mapped registers.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.mmio_types.all;
use work.vid_types.all;

entity mmio is
  generic(
    COLOR_BITS : positive;
    LOG2_VRAM_SIZE : positive;
    VIDEO_CONFIG : T_VIDEO_CONFIG
  );
  port(
    i_rst : in std_logic;

    -- Wishbone memory interface (b4 pipelined slave).
    -- See: https://cdn.opencores.org/downloads/wbspec_b4.pdf
    i_wb_clk : in std_logic;
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

    -- Some intput registers are collected externally.
    i_restart_frame : in std_logic;
    i_switches : in std_logic_vector(31 downto 0);
    i_buttons : in std_logic_vector(31 downto 0);

    -- All output registers are exported externally.
    o_regs_w: out T_MMIO_REGS_WO
  );
end mmio;

architecture rtl of mmio is
  -- System constants.
  constant C_CPU_CLK_HZ : integer := 70000000;  -- TODO(m): Implement me!
  constant C_VRAM_SIZE : integer := 2**LOG2_VRAM_SIZE;
  constant C_XRAM_SIZE : integer := 0;          -- TODO(m): Implement me!
  constant C_VID_WIDTH : integer := VIDEO_CONFIG.width;
  constant C_VID_HEIGHT : integer := VIDEO_CONFIG.height;
  constant C_VID_FPS : integer := 60*1000;      -- TODO(m): Implement me!

  -- Clock counter signals.
  signal s_clkcntlo_plus1 : unsigned(32 downto 0);
  signal s_clkcnthi_plus1 : unsigned(31 downto 0);

  -- Wishbone signals.
  signal s_request : std_logic;
  signal s_we : std_logic;

  -- Registers.
  signal s_regs_r : T_MMIO_REGS_RO;
  signal s_regs_w : T_MMIO_REGS_WO;
begin
  --------------------------------------------------------------------------------------------------
  -- Read-only registers.
  --------------------------------------------------------------------------------------------------

  -- Static read-only registers.
  s_regs_r.CPUCLK <= std_logic_vector(to_unsigned(C_CPU_CLK_HZ, 32));
  s_regs_r.VRAMSIZE <= std_logic_vector(to_unsigned(C_VRAM_SIZE, 32));
  s_regs_r.XRAMSIZE <= std_logic_vector(to_unsigned(C_XRAM_SIZE, 32));
  s_regs_r.VIDWIDTH <= std_logic_vector(to_unsigned(C_VID_WIDTH, 32));
  s_regs_r.VIDHEIGHT <= std_logic_vector(to_unsigned(C_VID_HEIGHT, 32));
  s_regs_r.VIDFPS <= std_logic_vector(to_unsigned(C_VID_FPS, 32));

  -- Increment the clock counter.
  s_clkcntlo_plus1 <= unsigned("0" & s_regs_r.CLKCNTLO) + to_unsigned(1, 33);
  s_clkcnthi_plus1 <= unsigned(s_regs_r.CLKCNTHI) + (31X"0" & s_clkcntlo_plus1(32 downto 32));

  -- Dynamic read-only registers.
  process(i_rst, i_wb_clk)
  begin
    if i_rst = '1' then
      s_regs_r.CLKCNTLO <= std_logic_vector(to_unsigned(0, 32));
      s_regs_r.CLKCNTHI <= std_logic_vector(to_unsigned(0, 32));
      s_regs_r.VIDFRAMENO <= std_logic_vector(to_unsigned(0, 32));
    elsif rising_edge(i_wb_clk) then
      -- Update the clock count.
      s_regs_r.CLKCNTLO <= std_logic_vector(s_clkcntlo_plus1(31 downto 0));
      s_regs_r.CLKCNTHI <= std_logic_vector(s_clkcnthi_plus1);

      -- Increment the frame count.
      if i_restart_frame = '1' then
        s_regs_r.VIDFRAMENO <= std_logic_vector(unsigned(s_regs_r.VIDFRAMENO) + to_unsigned(1, 32));
      end if;
    end if;
  end process;

  -- External read-only registers.
  s_regs_r.SWITCHES <= i_switches;
  s_regs_r.BUTTONS <= i_buttons;


  --------------------------------------------------------------------------------------------------
  -- Wishbone interface.
  --------------------------------------------------------------------------------------------------

  s_request <= i_wb_cyc and i_wb_stb;
  s_we <= s_request and i_wb_we;

  process(i_rst, i_wb_clk)
  begin
    if i_rst = '1' then
    elsif rising_edge(i_wb_clk) then
      -- Write-only registers.
      if s_we = '1' then
        if i_wb_adr(5 downto 2) = X"0" then
          s_regs_w.SEGDISP0 <= i_wb_dat;
        elsif i_wb_adr(5 downto 2) = X"1" then
          s_regs_w.SEGDISP1 <= i_wb_dat;
        elsif i_wb_adr(5 downto 2) = X"2" then
          s_regs_w.SEGDISP2 <= i_wb_dat;
        elsif i_wb_adr(5 downto 2) = X"3" then
          s_regs_w.SEGDISP3 <= i_wb_dat;
        elsif i_wb_adr(5 downto 2) = X"4" then
          s_regs_w.SEGDISP4 <= i_wb_dat;
        elsif i_wb_adr(5 downto 2) = X"5" then
          s_regs_w.SEGDISP5 <= i_wb_dat;
        elsif i_wb_adr(5 downto 2) = X"6" then
          s_regs_w.SEGDISP6 <= i_wb_dat;
        elsif i_wb_adr(5 downto 2) = X"7" then
          s_regs_w.SEGDISP7 <= i_wb_dat;
        elsif i_wb_adr(5 downto 2) = X"8" then
          s_regs_w.LEDS <= i_wb_dat;
        end if;
      end if;

      -- Read-only registers.
      if i_wb_adr(5 downto 2) = X"0" then
        o_wb_dat <= s_regs_r.CLKCNTLO;
      elsif i_wb_adr(5 downto 2) = X"1" then
        o_wb_dat <= s_regs_r.CLKCNTHI;
      elsif i_wb_adr(5 downto 2) = X"2" then
        o_wb_dat <= s_regs_r.CPUCLK;
      elsif i_wb_adr(5 downto 2) = X"3" then
        o_wb_dat <= s_regs_r.VRAMSIZE;
      elsif i_wb_adr(5 downto 2) = X"4" then
        o_wb_dat <= s_regs_r.XRAMSIZE;
      elsif i_wb_adr(5 downto 2) = X"5" then
        o_wb_dat <= s_regs_r.VIDWIDTH;
      elsif i_wb_adr(5 downto 2) = X"6" then
        o_wb_dat <= s_regs_r.VIDHEIGHT;
      elsif i_wb_adr(5 downto 2) = X"7" then
        o_wb_dat <= s_regs_r.VIDFPS;
      elsif i_wb_adr(5 downto 2) = X"8" then
        o_wb_dat <= s_regs_r.VIDFRAMENO;
      elsif i_wb_adr(5 downto 2) = X"9" then
        o_wb_dat <= s_regs_r.SWITCHES;
      elsif i_wb_adr(5 downto 2) = X"a" then
        o_wb_dat <= s_regs_r.BUTTONS;
      else
        o_wb_dat <= (others => '0');
      end if;

      -- Instant ack!
      o_wb_ack <= s_request;
    end if;
  end process;

  o_wb_err <= '0';
  o_wb_stall <= '0';


  --------------------------------------------------------------------------------------------------
  -- Output the state of the written registers.
  --------------------------------------------------------------------------------------------------

  o_regs_w <= s_regs_w;
end rtl;
