----------------------------------------------------------------------------------------------------
-- Copyright (c) 2022 Marcus Geelnard
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

entity sdram_controller_tb is
  generic (runner_cfg : string);
end entity;

architecture tb of sdram_controller_tb is
  constant C_CLK_HZ : integer := 100_000_000;
  constant C_CLK_HALF_PERIOD : time := 1000 ms / (2 * C_CLK_HZ);

  constant C_DATA_WIDTH : integer := 32;
  constant C_ADDR_WIDTH : integer := 24;

  signal s_rst : std_logic;
  signal s_clk : std_logic;

  signal s_adr : std_logic_vector(C_ADDR_WIDTH-1 downto 0);
  signal s_dat_w : std_logic_vector(C_DATA_WIDTH-1 downto 0);
  signal s_we : std_logic;
  signal s_sel : std_logic_vector(C_DATA_WIDTH/8-1 downto 0);
  signal s_req : std_logic;
  signal s_busy : std_logic;
  signal s_ack : std_logic;
  signal s_dat : std_logic_vector(C_DATA_WIDTH-1 downto 0);

  signal s_sdram_clk : std_logic;
  signal s_sdram_a : std_logic_vector(12 downto 0);
  signal s_sdram_ba : std_logic_vector(1 downto 0);
  signal s_sdram_dq : std_logic_vector(15 downto 0);
  signal s_sdram_cs_n : std_logic;
  signal s_sdram_cke : std_logic;
  signal s_sdram_ras_n : std_logic;
  signal s_sdram_cas_n : std_logic;
  signal s_sdram_we_n : std_logic;
  signal s_sdram_dqm : std_logic_vector(1 downto 0);
begin
  -- Instantiate the SDRAM controller.
  sdram_controller_1: entity work.sdram_controller
    generic map (
      G_CLK_FREQ_HZ => C_CLK_HZ,
      G_DATA_WIDTH => s_dat'length,
      G_ADDR_WIDTH => s_adr'length,
      G_SDRAM_A_WIDTH => s_sdram_a'length,
      G_SDRAM_DQ_WIDTH => s_sdram_dq'length,
      G_SDRAM_BA_WIDTH => s_sdram_ba'length,
      G_SDRAM_COL_WIDTH => 10,                -- 1k cols
      G_SDRAM_ROW_WIDTH => 13,                -- 8k rows
      G_T_DESL => 1000.0,     -- Use shorter wait times to speed up init
      G_T_REF => 3_000_000.0  -- Use shorter refresh intervals to inject refreshes
    )
    port map (
      i_rst  => s_rst,
      i_clk => s_clk,

      i_adr => s_adr,
      i_dat_w => s_dat_w,
      i_we => s_we,
      i_sel => s_sel,
      i_req => s_req,
      o_busy => s_busy,
      o_ack => s_ack,
      o_dat => s_dat,

      o_sdram_a => s_sdram_a,
      o_sdram_ba => s_sdram_ba,
      io_sdram_dq => s_sdram_dq,
      o_sdram_cke => s_sdram_cke,
      o_sdram_cs_n => s_sdram_cs_n,
      o_sdram_ras_n => s_sdram_ras_n,
      o_sdram_cas_n => s_sdram_cas_n,
      o_sdram_we_n => s_sdram_we_n,
      o_sdram_dqm => s_sdram_dqm
    );

  -- SDRAM - Simulate an SDRAM device.
  sdram_model_1: entity work.sdram_model
    generic map (
      ADDR_WIDTH => s_sdram_a'length,
      DATA_WIDTH => s_sdram_dq'length,
      COL_WIDTH => 10,                -- 1k cols
      ROW_WIDTH => 13,                -- 8k rows
      BANK_WIDTH => s_sdram_ba'length
    )
    port map (
      i_rst => s_rst,
      i_clk => s_sdram_clk,
      i_a => s_sdram_a,
      i_ba => s_sdram_ba,
      io_dq => s_sdram_dq,
      i_cke => s_sdram_cke,
      i_cs_n => s_sdram_cs_n,
      i_ras_n => s_sdram_ras_n,
      i_cas_n => s_sdram_cas_n,
      i_we_n => s_sdram_we_n,
      i_dqm => s_sdram_dqm
    );

  -- The SDRAM clock is 180 degrees phase delayed (for simplicity).
  s_sdram_clk <= not s_clk;

  main : process
    --  The requests to run and their expected responses.
    type T_REQ_TYPE is (NOP, RD, WR);
    type T_REQ is record
      -- Inputs.
      req_type : T_REQ_TYPE;
      adr : std_logic_vector(C_ADDR_WIDTH-1 downto 0);
      dat_w : std_logic_vector(C_DATA_WIDTH-1 downto 0);
      sel : std_logic_vector(C_DATA_WIDTH/8-1 downto 0);

      -- Expected outputs.
      dat : std_logic_vector(C_DATA_WIDTH-1 downto 0);
    end record;
    type T_REQ_ARRAY is array (natural range <>) of T_REQ;
    constant C_REQUESTS : T_REQ_ARRAY := (
        -- Write words to new bank: PRE+ACT
        (WR,  24X"000400", 32X"12345678", "1111",  32X"--------"),
        (WR,  24X"000401", 32X"55555555", "1111",  32X"--------"),
        (WR,  24X"000402", 32X"87654321", "1111",  32X"--------"),
        (WR,  24X"000403", 32X"aaaaaaaa", "1111",  32X"--------"),

        -- Write bytes to new bank: PRE+ACT
        (WR,  24X"030010", 32X"00000011", "0001",  32X"--------"),
        (WR,  24X"030010", 32X"00002200", "0010",  32X"--------"),
        (WR,  24X"030010", 32X"00330000", "0100",  32X"--------"),
        (NOP, 24X"------", 32X"--------", "----",  32X"--------"),
        (WR,  24X"030010", 32X"44000000", "1000",  32X"--------"),

        -- Read words from active bank: No delay
        (RD,  24X"000400", 32X"--------", "1111",  32X"12345678"),
        (RD,  24X"000401", 32X"--------", "1111",  32X"55555555"),
        (RD,  24X"000402", 32X"--------", "1111",  32X"87654321"),
        (RD,  24X"000403", 32X"--------", "1111",  32X"aaaaaaaa"),

        -- Read the bytes a word from active bank: No delay
        (RD,  24X"030010", 32X"--------", "1111",  32X"44332211"),

        -- Write-after-read: Delay to compensate for CAS
        (WR,  24X"030010", 32X"98760000", "1100",  32X"--------"),

        -- Precharge-after-write (write to new row): Delay + PRE+ACT
        (WR,  24X"830010", 32X"14253612", "1111",  32X"--------"),

        -- Read-after-write: No delay
        (RD,  24X"830010", 32X"--------", "1111",  32X"14253612"),

        -- A few NOP:s at the end...
        (NOP, 24X"------", 32X"--------", "----",  32X"--------"),
        (NOP, 24X"------", 32X"--------", "----",  32X"--------")
      );
      variable v_current_req : integer;
      variable v_current_ack : integer;
      variable v_cycles_since_last_ack : integer;
      variable v_expected_dat : std_logic_vector(C_DATA_WIDTH-1 downto 0);
  begin
    test_runner_setup(runner, runner_cfg);

    -- Continue running even if we have failures (for easier debugging).
    set_stop_level(failure);

    -- Clear DUT input signals.
    s_adr <= (others => '0');
    s_dat_w <= (others => '0');
    s_we <= '0';
    s_sel <= (others => '0');
    s_req <= '0';

    -- Reset the DUT.
    s_rst <= '1';
    s_clk <= '0';
    wait for C_CLK_HALF_PERIOD;
    s_clk <= '1';
    wait for C_CLK_HALF_PERIOD;
    s_rst <= '0';
    s_clk <= '0';
    wait for C_CLK_HALF_PERIOD;

    -- Walk through the requests.
    v_current_req := 0;
    v_current_ack := 0;
    v_cycles_since_last_ack := 0;
    while v_current_ack < C_REQUESTS'length loop
      -- Positive clock flank.
      s_clk <= '1';

      -- Wait for signals to stabilize.
      wait for C_CLK_HALF_PERIOD * 0.1;

      -- Start a new request?
      s_req <= '0';
      if v_current_req < C_REQUESTS'length then
        if s_busy = '0' then
          if C_REQUESTS(v_current_req).req_type /= NOP then
            s_req <= '1';
            s_adr <= C_REQUESTS(v_current_req).adr;
            s_dat_w <= C_REQUESTS(v_current_req).dat_w;
            if C_REQUESTS(v_current_req).req_type = WR then
              s_we <= '1';
            else
              s_we <= '0';
            end if;
            s_sel <= C_REQUESTS(v_current_req).sel;
          else
            s_req <= '0';
            s_adr <= (others => '-');
            s_dat_w <= (others => '-');
            s_we <= '-';
          end if;

          v_current_req := v_current_req + 1;
        else
          s_req <= '0';
          s_adr <= (others => '-');
          s_dat_w <= (others => '-');
          s_we <= '-';
        end if;
      end if;

      -- Check the output data.
      if C_REQUESTS(v_current_ack).req_type = NOP then
        v_current_ack := v_current_ack + 1;
      elsif s_ack = '1' then
        v_expected_dat := C_REQUESTS(v_current_ack).dat;
        if v_expected_dat /= 32X"--------" then
          check(s_dat = v_expected_dat, "Read data is incorrect");
        end if;

        v_current_ack := v_current_ack + 1;
        v_cycles_since_last_ack := 0;
      else
        v_cycles_since_last_ack := v_cycles_since_last_ack + 1;
      end if;

      -- Safeguard against missing acks.
      assert v_cycles_since_last_ack < 500 report "Missing ack?";

      -- Tick the clock.
      wait for C_CLK_HALF_PERIOD * 0.9;
      s_clk <= '0';
      wait for C_CLK_HALF_PERIOD;
    end loop;

    test_runner_cleanup(runner);
  end process;
end architecture;
