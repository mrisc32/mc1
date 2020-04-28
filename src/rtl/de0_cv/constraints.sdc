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

# Set Clock Uncertainty
derive_clock_uncertainty
