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
-- This is the top level entity of the MC1.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use work.vid_types.all;

entity mc1 is
  generic(
    -- Note: The default configuration is conservative. Be sure to pass in values that are suitable
    -- for your target platform.
    LOG2_VRAM_SIZE : positive := 12;  -- VRAM size (log2 of number of 32-bit words).
    VIDEO_CONFIG : T_VIDEO_CONFIG     -- Native video resolution.
  );
  port(
    -- CPU interface.
    i_cpu_rst : in std_logic;
    i_cpu_clk : in std_logic;

    -- VGA interface.
    i_vga_rst : in std_logic;
    i_vga_clk : in std_logic;
    o_vga_r : out std_logic_vector(7 downto 0);
    o_vga_g : out std_logic_vector(7 downto 0);
    o_vga_b : out std_logic_vector(7 downto 0);
    o_vga_hs : out std_logic;
    o_vga_vs : out std_logic;

    -- I/O: Generic input and output registers.
    i_io: in std_logic_vector(31 downto 0);
    o_io: out std_logic_vector(31 downto 0)
  );
end mc1;

architecture rtl of mc1 is
  -- CPU memory interface (Wishbone B4 pipelined master).
  signal s_cpu_cyc : std_logic;
  signal s_cpu_stb : std_logic;
  signal s_cpu_adr : std_logic_vector(31 downto 2);
  signal s_cpu_dat_w : std_logic_vector(31 downto 0);
  signal s_cpu_we : std_logic;
  signal s_cpu_sel : std_logic_vector(3 downto 0);
  signal s_cpu_dat : std_logic_vector(31 downto 0);
  signal s_cpu_ack : std_logic;
  signal s_cpu_stall : std_logic;
  signal s_cpu_err : std_logic;

  -- ROM memory interface (Wishbone B4 pipelined slave).
  signal s_rom_cyc : std_logic;
  signal s_rom_stb : std_logic;
  signal s_rom_adr : std_logic_vector(31 downto 2);
  signal s_rom_dat : std_logic_vector(31 downto 0);
  signal s_rom_ack : std_logic;
  signal s_rom_stall : std_logic;
  signal s_rom_err : std_logic;

  -- Internal VRAM memory interface (Wishbone B4 pipelined slave).
  signal s_vram_cyc : std_logic;
  signal s_vram_stb : std_logic;
  signal s_vram_adr : std_logic_vector(31 downto 2);
  signal s_vram_dat_w : std_logic_vector(31 downto 0);
  signal s_vram_we : std_logic;
  signal s_vram_sel : std_logic_vector(3 downto 0);
  signal s_vram_dat : std_logic_vector(31 downto 0);
  signal s_vram_ack : std_logic;
  signal s_vram_stall : std_logic;
  signal s_vram_err : std_logic;

  -- Video logic signals.
  signal s_video_adr : std_logic_vector(LOG2_VRAM_SIZE-1 downto 0);
  signal s_video_dat : std_logic_vector(31 downto 0);
  signal s_video_r : std_logic_vector(7 downto 0);
  signal s_video_g : std_logic_vector(7 downto 0);
  signal s_video_b : std_logic_vector(7 downto 0);
  signal s_video_hsync : std_logic;
  signal s_video_vsync : std_logic;

  -- Memory mapped I/O interface (Wishbone B4 pipelined slave).
  signal s_io_cyc : std_logic;
  signal s_io_stb : std_logic;
  signal s_io_adr : std_logic_vector(31 downto 2);
  signal s_io_dat_w : std_logic_vector(31 downto 0);
  signal s_io_we : std_logic;
  signal s_io_sel : std_logic_vector(3 downto 0);
  signal s_io_dat : std_logic_vector(31 downto 0);
  signal s_io_ack : std_logic;
  signal s_io_stall : std_logic;
  signal s_io_err : std_logic;
begin
  --------------------------------------------------------------------------------------------------
  -- CPU core
  --------------------------------------------------------------------------------------------------
  mrisc32_core_1: entity work.core
    port map (
      i_clk => i_cpu_clk,
      i_rst => i_cpu_rst,

      -- Data interface.
      o_wb_cyc => s_cpu_cyc,
      o_wb_stb => s_cpu_stb,
      o_wb_adr => s_cpu_adr,
      o_wb_dat => s_cpu_dat_w,
      o_wb_we => s_cpu_we,
      o_wb_sel => s_cpu_sel,
      i_wb_dat => s_cpu_dat,
      i_wb_ack => s_cpu_ack,
      i_wb_stall => s_cpu_stall,
      i_wb_err => s_cpu_err
    );

  --------------------------------------------------------------------------------------------------
  -- Wishbone memory subsystem
  --------------------------------------------------------------------------------------------------
  memory_mux_1: entity work.memory_mux
    port map (
      i_rst => i_cpu_rst,
      i_clk => i_cpu_clk,

      -- Wishbone master interface from the CPU.
      i_wb_cyc => s_cpu_cyc,
      i_wb_stb => s_cpu_stb,
      i_wb_adr => s_cpu_adr,
      i_wb_dat => s_cpu_dat_w,
      i_wb_we => s_cpu_we,
      i_wb_sel => s_cpu_sel,
      o_wb_dat => s_cpu_dat,
      o_wb_ack => s_cpu_ack,
      o_wb_stall => s_cpu_stall,
      o_wb_err => s_cpu_err,

      -- Wishbone slave interface 0: ROM.
      o_wb_cyc_0 => s_rom_cyc,
      o_wb_stb_0 => s_rom_stb,
      o_wb_adr_0 => s_rom_adr,
      i_wb_dat_0 => s_rom_dat,
      i_wb_ack_0 => s_rom_ack,
      i_wb_stall_0 => s_rom_stall,
      i_wb_err_0 => s_rom_err,

      -- Wishbone slave interface 1: Internal VRAM.
      o_wb_cyc_1 => s_vram_cyc,
      o_wb_stb_1 => s_vram_stb,
      o_wb_adr_1 => s_vram_adr,
      o_wb_dat_1 => s_vram_dat_w,
      o_wb_we_1 => s_vram_we,
      o_wb_sel_1 => s_vram_sel,
      i_wb_dat_1 => s_vram_dat,
      i_wb_ack_1 => s_vram_ack,
      i_wb_stall_1 => s_vram_stall,
      i_wb_err_1 => s_vram_err,

      -- External RAM interface
      -- TODO(m): Implement me!
      i_wb_dat_2 => (others => '0'),
      i_wb_ack_2 => '0',
      i_wb_stall_2 => '0',
      i_wb_err_2 => '0',

      -- Memory mapped I/O interface.
      o_wb_cyc_3 => s_io_cyc,
      o_wb_stb_3 => s_io_stb,
      o_wb_adr_3 => s_io_adr,
      o_wb_dat_3 => s_io_dat_w,
      o_wb_we_3 => s_io_we,
      o_wb_sel_3 => s_io_sel,
      i_wb_dat_3 => s_io_dat,
      i_wb_ack_3 => s_io_ack,
      i_wb_stall_3 => s_io_stall,
      i_wb_err_3 => s_io_err
    );

  -- Internal ROM.
  rom_1: entity work.rom
    port map (
      i_clk => i_cpu_clk,

      i_wb_cyc => s_rom_cyc,
      i_wb_stb => s_rom_stb,
      i_wb_adr => s_rom_adr,
      o_wb_dat => s_rom_dat,
      o_wb_ack => s_rom_ack,
      o_wb_stall => s_rom_stall
    );
  s_rom_err <= '0';

  -- Internal VRAM.
  vram_1: entity work.vram
    generic map (
      ADR_BITS => LOG2_VRAM_SIZE
    )
    port map (
      i_rst => i_cpu_rst,

      -- CPU interface.
      i_wb_clk => i_cpu_clk,
      i_wb_cyc => s_vram_cyc,
      i_wb_stb => s_vram_stb,
      i_wb_adr => s_vram_adr(LOG2_VRAM_SIZE+1 downto 2),
      i_wb_dat => s_vram_dat_w,
      i_wb_we => s_vram_we,
      i_wb_sel => s_vram_sel,
      o_wb_dat => s_vram_dat,
      o_wb_ack => s_vram_ack,
      o_wb_stall => s_vram_stall,

      -- Video interface.
      i_read_clk => i_vga_clk,
      i_read_adr => s_video_adr,
      o_read_dat => s_video_dat
    );
  s_vram_err <= '0';

  --------------------------------------------------------------------------------------------------
  -- Video logic
  --------------------------------------------------------------------------------------------------
  video_1: entity work.video
    generic map (
      ADR_BITS => LOG2_VRAM_SIZE,
      VIDEO_CONFIG => VIDEO_CONFIG
    )
    port map (
      i_rst => i_vga_rst,
      i_clk => i_vga_clk,

      o_read_adr => s_video_adr,
      i_read_dat => s_video_dat,

      o_r => s_video_r,
      o_g => s_video_g,
      o_b => s_video_b,

      o_hsync => s_video_hsync,
      o_vsync => s_video_vsync
    );

  o_vga_r <= s_video_r;
  o_vga_g <= s_video_g;
  o_vga_b <= s_video_b;
  o_vga_hs <= s_video_hsync;
  o_vga_vs <= s_video_vsync;

  --------------------------------------------------------------------------------------------------
  -- I/O
  --------------------------------------------------------------------------------------------------

  process (i_cpu_clk, i_cpu_rst)
  begin
    if i_cpu_rst = '1' then
      s_io_dat <= (others => '0');
      s_io_ack <= '0';
    elsif rising_edge(i_cpu_clk) then
      if s_io_cyc = '1' and s_io_stb = '1' then
        -- Note: We currently do not care about the address.
        if s_io_we = '1' then
          o_io <= s_io_dat_w;
        else
          s_io_dat <= i_io;
        end if;
      end if;
      s_io_ack <= s_io_cyc and s_io_stb;
    end if;
  end process;

  s_io_err <= '0';
  s_io_stall <= '0';

end rtl;
