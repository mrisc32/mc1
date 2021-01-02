# Constrain the input clock ports with a 20 ns requirement (50 MHz).
create_clock -period 20 [get_ports CLOCK_50]
create_clock -period 20 [get_ports CLOCK2_50]
create_clock -period 20 [get_ports CLOCK3_50]
create_clock -period 20 [get_ports CLOCK4_50]

# Constrain the GPIO-0 pin 1 as an input clock port with a 20 ns requirement (50 MHz).
create_clock -period 20 [get_ports GPIO_0[0]]

# Automatically apply a generate clock on the output of phase-locked loops (PLLs).
# This command can be safely left in the SDC even if no PLLs exist in the design.
derive_pll_clocks

# The PLL:s generate three main clocks.
set cpu_pll   "pll_cpu|pll_1|altera_pll_i|general[0].gpll~PLL_OUTPUT_COUNTER|divclk"
set sdram_pll "pll_cpu|pll_1|altera_pll_i|general[1].gpll~PLL_OUTPUT_COUNTER|divclk"
set vga_pll   "pll_vga|pll_1|altera_pll_i|general[0].gpll~PLL_OUTPUT_COUNTER|divclk"

# SDRAM clock.
create_generated_clock -name sdram_clk -source $sdram_pll [get_ports {DRAM_CLK}]

# Set Clock Uncertainty
derive_clock_uncertainty

# SDRAM timing.
set sdram_tsu       1.5
set sdram_th        0.8
set sdram_tco_min   2.7
set sdram_tco_max   5.4

# SDRAM timing constraints.
set sdram_input_delay_min   $sdram_tco_min
set sdram_input_delay_max   $sdram_tco_max
set sdram_output_delay_min -$sdram_th
set sdram_output_delay_max  $sdram_tsu

# PLL to SDRAM output (clear the unconstrained path warning).
set_min_delay -from $sdram_pll -to [get_ports {DRAM_CLK}] 1
set_max_delay -from $sdram_pll -to [get_ports {DRAM_CLK}] 6

# SDRAM outputs.
set sdram_outputs [get_ports {
	DRAM_CKE
	DRAM_ADDR[*]
	DRAM_BA[*]
	DRAM_DQ[*]
	DRAM_CS_N
	DRAM_RAS_N
	DRAM_CAS_N
	DRAM_WE_N
	DRAM_LDQM
	DRAM_UDQM
}]
set_output_delay \
	-clock sdram_clk \
	-min $sdram_output_delay_min \
	$sdram_outputs
set_output_delay \
	-clock sdram_clk \
	-max $sdram_output_delay_max \
	$sdram_outputs

# SDRAM inputs.
set sdram_inputs [get_ports {
	DRAM_DQ[*]
}]
set_input_delay \
	-clock sdram_clk \
	-min $sdram_input_delay_min \
	$sdram_inputs
set_input_delay \
	-clock sdram_clk \
	-max $sdram_input_delay_max \
	$sdram_inputs

# SDRAM-to-FPGA multi-cycle constraint.
#
# * The PLL is configured so that SDRAM clock leads the CPU clock.
set_multicycle_path -setup -end -from sdram_clk -to $cpu_pll 2

