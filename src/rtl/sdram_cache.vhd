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

----------------------------------------------------------------------------------------------------
-- This is an XRAM implementation for SDRAM memories. It has a configurable set associative
-- write-back cache to reduce the effect of SDRAM latency.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;
use work.config.all;

entity sdram_cache is
  generic (
    -- Clock frequency (in Hz)
    CPU_CLK_HZ : integer;

    -- See sdram.vhd for the details of these generics.
    SDRAM_ADDR_WIDTH : natural := 13;
    SDRAM_DATA_WIDTH : natural := 16;
    SDRAM_COL_WIDTH : natural := 9;
    SDRAM_ROW_WIDTH : natural := 13;
    SDRAM_BANK_WIDTH : natural := 2;
    CAS_LATENCY : natural := 2;
    T_DESL : real := 200000.0;
    T_MRD : real := 12.0;
    T_RC : real := 60.0;
    T_RCD : real := 18.0;
    T_RP : real := 18.0;
    T_WR : real := 12.0;
    T_REFI : real := 7800.0;

    -- Cache size configuration.
    LOG2_WORDS_PER_LINE : integer := 2;   -- 16 bytes / cache line
    LOG2_WAYS : integer := 2;             -- 4-way
    LOG2_SETS : integer := 8              -- 256 sets
  );
  port (
    -- Reset signal.
    i_rst : in std_logic;

    -- Wishbone memory interface (b4 pipelined slave).
    -- See: https://cdn.opencores.org/downloads/wbspec_b4.pdf
    i_wb_clk : in std_logic;
    i_wb_cyc : in std_logic;
    i_wb_stb : in std_logic;
    i_wb_adr : in std_logic_vector(29 downto 0);
    i_wb_dat : in std_logic_vector(31 downto 0);
    i_wb_we : in std_logic;
    i_wb_sel : in std_logic_vector(32/8-1 downto 0);
    o_wb_dat : out std_logic_vector(31 downto 0);
    o_wb_ack : out std_logic;
    o_wb_stall : out std_logic;
    o_wb_err : out std_logic;

    -- SDRAM interface.
    o_sdram_a : out std_logic_vector(SDRAM_ADDR_WIDTH-1 downto 0);
    o_sdram_ba : out std_logic_vector(SDRAM_BANK_WIDTH-1 downto 0);
    io_sdram_dq : inout std_logic_vector(SDRAM_DATA_WIDTH-1 downto 0);
    o_sdram_cke : out std_logic;
    o_sdram_cs_n : out std_logic;
    o_sdram_ras_n : out std_logic;
    o_sdram_cas_n : out std_logic;
    o_sdram_we_n : out std_logic;
    o_sdram_dqm : out std_logic_vector(SDRAM_DATA_WIDTH/8-1 downto 0)
  );
end sdram_cache;

architecture rtl of sdram_cache is
  function log2(x: integer) return integer is
  begin
    if x <= 1 then
      return 0;
    else
      return 1 + log2(x/2)
    end if;
  end function;

  -- Cache size in bytes = 4 * C_WORDS_PER_LINE * C_WAYS * C_SETS
  constant C_WORDS_PER_LINE : integer := 2 ** LOG2_WORDS_PER_LINE;
  constant C_WAYS : integer := 2**LOG2_WAYS;
  constant C_SETS : integer := 2**LOG2_SETS;

  constant C_LOG2_LINE_WIDTH : integer := LOG2_WORDS_PER_LINE + 5;  -- 32-bit words.
  constant C_LINE_WIDTH : integer := 2 ** C_LOG2_LINE_WIDTH;

  -- Effective address width: log2 of number of cache lines in SDRAM.
  constant C_ADDR_WIDTH : natural := SDRAM_COL_WIDTH + SDRAM_ROW_WIDTH + SDRAM_BANK_WIDTH + log2(SDRAM_DATA_WIDTH) - C_LOG2_LINE_WIDTH;

  -- The tag only needs to consider the address bits that map to the SDRAM address space,
  -- so we strip the most significant bits (they only provide additional mirrors).
  constant C_TAG_WIDTH : integer := C_ADDR_WIDTH - LOG2_SETS;

  -- Each cache line contains (in this order, MSB to LSB):
  --  * Valid flag: 1 bit
  --  * Dirty flag: 1 bit
  --  * Tag
  --  * Data
  constant C_ENTRY_WIDTH : integer := 1 + 1 + C_TAG_WIDTH + C_LINE_WIDTH;

  -- We burst an entire cache line from/to SDRAM in each transaction.
  constant C_BURST_LENGTH : integer := C_LINE_WIDTH / SDRAM_DATA_WIDTH;

  subtype T_LINE is std_logic_vector(C_LINE_WIDTH-1 downto 0);
  subtype T_TAG is std_logic_vector(C_TAG_WIDTH-1 downto 0);
  subtype T_CACHE_ENTRY is std_logic_vector(C_ENTRY_WIDTH-1 downto 0);
  subtype T_CACHE_ADDR is std_logic_vector(LOG2_SETS-1 downto 0);

  -- State machine states.
  type T_STATE is (
    INVALIDATE,
    READY,
    WAIT_FOR_MEM_WRITE,
    WAIT_FOR_MEM_READ
  );

  signal s_next_state : T_STATE;
  signal s_state : T_STATE;

  signal s_do_lookup : std_logic;
  signal s_lookup_adr : std_logic_vector(C_WORD_SIZE-3 downto 0);
  signal s_lookup_dat : std_logic_vector(C_WORD_SIZE-1 downto 0);
  signal s_lookup_we : std_logic;
  signal s_lookup_sel : std_logic_vector(C_WORD_SIZE/8-1 downto 0);

  signal s_cache_hit_way : unsigned(LOG2_SETS-1 downto 0);
  signal s_cache_miss : std_logic;

  signal s_next_invalidate_line_no : unsigned(LOG2_SETS-1 downto 0);
  signal s_invalidate_line_no : unsigned(LOG2_SETS-1 downto 0);

  signal s_rnd_way : unsigned(LOG2_WAYS-1 downto 0);
  signal s_next_target_way : unsigned(LOG2_WAYS-1 downto 0);
  signal s_target_way : unsigned(LOG2_WAYS-1 downto 0);

  signal s_write_data : T_CACHE_ENTRY;
  signal s_write_addr : T_CACHE_ADDR;
  signal s_write_en : std_logic_vector(C_WAYS-1 downto 0);

  signal s_write_data_final : T_CACHE_ENTRY;
  signal s_write_addr_final : T_CACHE_ADDR;
  signal s_write_en_final : std_logic_vector(C_WAYS-1 downto 0);

  type T_READ_ARRAY is array C_WAYS-1 downto 0 of T_CACHE_ENTRY;
  signal s_read_data : T_READ_ARRAY;
  signal s_read_addr : T_CACHE_ADDR;

  signal s_mem_addr : unsigned(C_ADDR_WIDTH-1 downto 0);
  signal s_mem_data : T_LINE;
  signal s_mem_we : std_logic;
  signal s_mem_sel : std_logic_vector((C_LINE_WIDTH/8)-1 downto 0);
  signal s_mem_req : std_logic;
  signal s_mem_ready : std_logic;
  signal s_mem_ack : std_logic;
  signal s_mem_valid : std_logic;
  signal s_mem_q : T_LINE;

  signal s_sdram_a : unsigned(SDRAM_ADDR_WIDTH-1 downto 0);
  signal s_sdram_ba : unsigned(SDRAM_BANK_WIDTH-1 downto 0);

  function is_valid(x: T_CACHE_ENTRY) return boolean is
  begin
    return x(C_ENTRY_WIDTH-1) = '1';
  end function;

  function is_dirty(x: T_CACHE_ENTRY) return boolean is
  begin
    return x(C_ENTRY_WIDTH-2) = '1';
  end function;

  function get_tag(x: T_CACHE_ENTRY) return T_TAG is
  begin
    return x(C_ENTRY_WIDTH-3 downto C_ENTRY_WIDTH-2-C_TAG_WIDTH);
  end function;

  function get_data(x: T_CACHE_ENTRY) return T_LINE is
  begin
    return x(C_LINE_WIDTH-1 downto 0);
  end function;

  function make_mem_addr(word_adr: std_logic_vector) return unsigned is
  begin
    return unsigned(word_adr(C_ADDR_WIDTH+LOG2_WORDS_PER_LINE-1 downto LOG2_WORDS_PER_LINE));
  end function;

  function make_mem_addr(word_adr: std_logic_vector, tag: T_TAG) return unsigned is
    variable v_mem_addr: unsigned;
  begin
    v_mem_addr := make_mem_addr(word_adr);
    return unsigned(tag) & v_mem_addr(C_ADDR_WIDTH-C_TAG_WIDTH-1 downto 0);
  end function;

  function make_cache_addr(word_adr: std_logic_vector) return std_logic_vector is
  begin
    return word_adr(LOG2_SETS+LOG2_WORDS_PER_LINE-1 downto LOG2_WORDS_PER_LINE);
  end function;

  function make_cache_tag(word_adr: std_logic_vector) return T_TAG is
  begin
    return word_adr(C_TAG_WIDTH+LOG2_SETS+LOG2_WORDS_PER_LINE-1 downto LOG2_SETS+LOG2_WORDS_PER_LINE);
  end function;

  function make_invalid_entry return T_CACHE_ENTRY is
  begin
    return std_logic_vector(to_unsigned(0, C_ENTRY_WIDTH));
  end function;

  function make_valid_entry(dirty: boolean, tag: T_TAG, data: T_LINE) return T_CACHE_ENTRY is
    variable v_cache_entry: T_CACHE_ENTRY;
  begin
    -- Valid
    v_cache_entry(C_ENTRY_WIDTH-1) := '1';

    -- Dirty
    if dirty then
      v_cache_entry(C_ENTRY_WIDTH-2) := '1';
    else
      v_cache_entry(C_ENTRY_WIDTH-2) := '0';
    end if;

    -- Tag
    v_cache_entry(C_ENTRY_WIDTH-3 downto C_ENTRY_WIDTH-2-C_TAG_WIDTH) := tag;

    -- Data
    v_cache_entry(C_LINE_WIDTH-1 downto 0) := data;
  end function;
begin
  -----------------------------------------------------------------------------
  -- Cache RAM.
  -----------------------------------------------------------------------------

  RAMGen: for way in 0 to C_WAYS-1 generate
  begin
    cache_ram: entity work.ram_dual_port
      generic map (
        WIDTH => C_ENTRY_WIDTH,
        ADDR_BITS => LOG2_SETS,
        PREFER_DISTRIBUTED => false
      )
      port map (
        i_clk => i_wb_clk,
        i_write_data => s_write_data_final,
        i_write_addr => s_write_addr_final,
        i_we => s_write_en_final(way),
        i_read_addr => s_read_addr,
        o_read_data => s_read_data(way)
      );
  end generate;


  -----------------------------------------------------------------------------
  -- Cache lookup.
  -----------------------------------------------------------------------------

  process (ALL)
    variable v_want_tag : T_TAG;
    variable v_found_hit : boolean;
    variable v_entry : T_CACHE_ENTRY;
    variable v_way : unsigned(LOG2_SETS-1 downto 0);
  begin
    -- If we did a lookup during the previous cycle, we have a result from the
    -- cache memory during this cycle.
    if s_do_lookup = '1' then
      -- Concurrently check the tags of all ways for the given set to see
      -- if we got a hit.
      v_want_tag := make_cache_tag(s_lookup_adr);
      v_found_hit := false;
      for k in 0 to C_WAYS-1 loop
        v_entry := s_read_data(k);
        if is_valid(v_entry) and get_tag(v_entry) = v_want_tag then
          v_way := to_unsigned(k, LOG2_WAYS);
          v_found_hit := true;
        end if;
      end loop;

      if v_found_hit then
        s_cache_hit_way <= v_way;
        s_cache_miss <= '0';
      else
        s_cache_hit_way <= (others => '0');
        s_cache_miss <= '1';
      end if;
    else
      s_cache_hit_way <= (others => '0');
      s_cache_miss <= '0';
    end if;
  end process;

  -- When there is a cache hit, we need to write/mix data according to i_wb_we etc.
  -- TODO(m): Implement me!
  s_write_data_final <= s_write_data;
  s_write_addr_final <= s_write_addr;
  s_write_en_final <= s_write_en;

  -- Respond back to the WB interface with cache hits.
  -- TODO(m): Implement me!
  o_wb_dat <= s_read_data(to_integer(s_cache_hit_way));
  o_wb_ack <= s_do_lookup and not s_cache_miss;
  o_wb_stall <= '0';
  o_wb_err <= '0';

  -- Latch requests from the Wishbone bus.
  process (i_rst, i_wb_clk)
  begin
    if i_rst = '1' then
      s_do_lookup <= '0';
      s_lookup_adr <= (others => '0');
      s_lookup_dat <= (others => '0');
      s_lookup_we <= (others => '0');
      s_lookup_sel <= (others => '0');
    elsif rising_edge(i_wb_clk) then
      if i_wb_cyc = '1' and i_wb_stb = '1' then
        s_do_lookup <= '1';
        s_lookup_adr <= i_wb_adr;
        s_lookup_dat <= i_wb_dat;
        s_lookup_we <= i_wb_we;
        s_lookup_sel <= i_wb_sel;
      else
        s_do_lookup <= '0';
      end if;
    end if;
  end process;


  -----------------------------------------------------------------------------
  -- State machine.
  -----------------------------------------------------------------------------

  process(ALL)
    variable v_line : T_LINE;
  begin
    case s_state is
      when INVALIDATE =>
        s_next_invalidate_line_no <= s_invalidate_line_no + 1;
        s_write_addr <= std_logic_vector(s_invalidate_line_no);
        s_write_data <= make_invalid_entry;
        s_write_en <= (others => '1');    -- Write to all ways at once.
        s_next_target_way <= s_target_way;

        s_mem_req <= '0';
        s_mem_we <= '0';
        s_mem_addr <= (others => '0');
        s_mem_data <= (others => '0');

        if s_invalidate_line_no = C_SETS-1 then
          s_next_state <= READY;
        else
          s_next_state <= INVALIDATE;
        end if;

      when READY =>
        s_next_invalidate_line_no <= (others => '0');
        s_write_addr <= (others => '0');
        s_write_data <= (others => '0');
        s_write_en <= (others => '0');

        -- TODO(m): If s_mem_ready = '0', we need to hold on to the s_cache_miss value
        -- for a few more cycles.
        if s_cache_miss = '1' and s_mem_ready = '1' then
          -- Pick a cache line to replace.
          -- TODO(m): Implement LRU.
          s_next_target_way <= s_rnd_way;
          v_line := s_read_data(to_integer(s_next_target_way));

          -- We need to write back the line if it's dirty, otherwise skip directly to reading it from mem.
          s_mem_req <= '1';
          if is_dirty(v_line) then
            s_mem_we <= '1';
            s_mem_addr <= make_mem_addr(word_adr => s_lookup_adr, tag => get_tag(v_line));
            s_mem_data <= get_data(v_line);
            s_next_state <= WAIT_FOR_MEM_WRITE;
          else
            s_mem_we <= '0';
            s_mem_addr <= make_mem_addr(word_adr => s_lookup_adr);
            s_mem_data <= (others => '0');
            s_next_state <= WAIT_FOR_MEM_READ;
          end if;
        else
          s_next_target_way <= s_target_way;
          s_mem_req <= '0';
          s_mem_we <= '0';
          s_mem_addr <= (others => '0');
          s_mem_data <= (others => '0');
          s_next_state <= READY;
        end if;

      when WAIT_FOR_MEM_WRITE =>
        s_next_invalidate_line_no <= (others => '0');
        s_write_addr <= (others => '0');
        s_write_data <= (others => '0');
        s_write_en <= (others => '0');
        s_next_target_way <= s_target_way;

        if s_mem_ready = '1' then
          s_mem_req <= '1';
          s_mem_we <= '0';
          s_mem_addr <= make_mem_addr(word_adr => s_lookup_adr);
          s_mem_data <= (others => '0');
          s_next_state <= WAIT_FOR_MEM_READ;
        else
          s_mem_req <= '0';
          s_mem_we <= '0';
          s_mem_addr <= (others => '0');
          s_mem_data <= (others => '0');
          s_next_state <= WAIT_FOR_MEM_WRITE;
        end if;

      when WAIT_FOR_MEM_READ =>
        s_next_invalidate_line_no <= (others => '0');
        s_write_addr <= make_cache_addr(s_lookup_adr);
        -- TODO(m): Mix s_mem_q with s_lookup_dat according to s_lookup_sel & s_lookup_we.
        s_write_data <= make_valid_entry(dirty => true,
                                         tag => make_tag(s_lookup_adr),
                                         data => s_mem_q);
        for k in 0 to C_WAYS-1 loop
          if to_unsigned(k, LOG2_WAYS) = s_target_way then
            s_write_en(k) <= s_mem_valid;
          else
            s_write_en(k) <= '0';
          end if;
        end loop;
        s_next_target_way <= s_target_way;

        s_mem_req <= '0';
        s_mem_we <= '0';
        s_mem_addr <= (others => '0');
        s_mem_data <= (others => '0');

        if s_mem_valid = '1' then
          -- TODO(m): Service cache misses immediately.
          s_next_state <= READY;
        else
          s_next_state <= WAIT_FOR_MEM_READ;
        end if;

      when others =>
        s_next_invalidate_line_no <= (others => '0');
        s_write_addr <= (others => '0');
        s_write_data <= (others => '0');
        s_write_en <= (others => '0');
        s_next_target_way <= s_target_way;

        s_mem_req <= '0';
        s_mem_we <= '0';
        s_mem_addr <= (others => '0');
        s_mem_data <= (others => '0');

        s_next_state <= RESET;
    end case;
  end process;

  process(i_wb_clk, i_rst)
  begin
    if i_rst = '1' then
      s_invalidate_line_no <= (others => '0');
      s_state <= INVALIDATE;
      s_rnd_way <= (others => '0');
      s_target_way <= (others => '0');
    elsif rising_edge(i_wb_clk) then
      s_invalidate_line_no <= s_next_invalidate_line_no;
      s_state <= s_next_state;
      s_rnd_way <= s_rnd_way + 1;
      s_target_way <= s_next_target_way;
    end if;
  end process;


  -----------------------------------------------------------------------------
  -- SDRAM.
  -----------------------------------------------------------------------------

  -- We always write all bytes of the cache line.
  s_mem_sel <= (others => '1');

  -- Convert some SDRAM outputs to SLV.
  o_sdram_a <= std_logic_vector(s_sdram_a);
  o_sdram_ba <= std_logic_vector(s_sdram_ba);

  -- Instantiate the SDRAM controller.
  sdram_controller_1: entity work.sdram
    generic map (
      CLK_FREQ => real(CPU_CLK_HZ)*0.000001,
      ADDR_WIDTH => C_ADDR_WIDTH,
      DATA_WIDTH => C_LINE_WIDTH,
      SDRAM_ADDR_WIDTH => SDRAM_ADDR_WIDTH,
      SDRAM_DATA_WIDTH => SDRAM_DATA_WIDTH,
      SDRAM_COL_WIDTH => SDRAM_COL_WIDTH,
      SDRAM_ROW_WIDTH => SDRAM_ROW_WIDTH,
      SDRAM_BANK_WIDTH => SDRAM_BANK_WIDTH,
      CAS_LATENCY => CAS_LATENCY,
      BURST_LENGTH => C_BURST_LENGTH,
      T_DESL => T_DESL,
      T_MRD => T_MRD,
      T_RC => T_RC,
      T_RCD => T_RCD,
      T_RP => T_RP,
      T_WR => T_WR,
      T_REFI => T_REFI
    )
    port map (
      reset  => i_rst,
      clk => i_wb_clk,

      -- CPU/Wishbone interface.
      addr => s_mem_addr,
      data => s_mem_data,
      we => s_mem_we,
      sel => s_mem_sel,
      req => s_mem_req,
      ready => s_mem_ready,
      ack => s_mem_ack,
      valid => s_mem_valid,
      q => s_mem_q,

      -- External SDRAM interface.
      sdram_a => s_sdram_a,
      sdram_ba => s_sdram_ba,
      sdram_dq => io_sdram_dq,
      sdram_cke => o_sdram_cke,
      sdram_cs_n => o_sdram_cs_n,
      sdram_ras_n => o_sdram_ras_n,
      sdram_cas_n => o_sdram_cas_n,
      sdram_we_n => o_sdram_we_n,
      sdram_dqm => o_sdram_dqm
    );

end architecture rtl;
