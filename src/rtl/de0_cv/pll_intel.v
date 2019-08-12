//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// This is a simple parameterized Intel flavor PLL. It infers an "altera_pll" IP that exposes up to
// eight output clocks (o_clk0 to o_clk7).
//--------------------------------------------------------------------------------------------------

`timescale 1ns/10ps
module pll_intel(
  input wire i_rst,
  input wire i_refclk,
  output wire o_locked,
  output wire o_clk0,
  output wire o_clk1,
  output wire o_clk2,
  output wire o_clk3,
  output wire o_clk4,
  output wire o_clk5,
  output wire o_clk6,
  output wire o_clk7
);

  parameter REFERENCE_CLOCK_FREQUENCY = "100.0 MHz";
  parameter NUMBER_OF_CLOCKS = 1;
  parameter OUTPUT_CLOCK_FREQUENCY0 = "100.0 MHz";
  parameter PHASE_SHIFT0 = "0 ps";
  parameter DUTY_CYCLE0 = 50;
  parameter OUTPUT_CLOCK_FREQUENCY1 = "0 MHz";
  parameter PHASE_SHIFT1 = "0 ps";
  parameter DUTY_CYCLE1 = 50;
  parameter OUTPUT_CLOCK_FREQUENCY2 = "0 MHz";
  parameter PHASE_SHIFT2 = "0 ps";
  parameter DUTY_CYCLE2 = 50;
  parameter OUTPUT_CLOCK_FREQUENCY3 = "0 MHz";
  parameter PHASE_SHIFT3 = "0 ps";
  parameter DUTY_CYCLE3 = 50;
  parameter OUTPUT_CLOCK_FREQUENCY4 = "0 MHz";
  parameter PHASE_SHIFT4 = "0 ps";
  parameter DUTY_CYCLE4 = 50;
  parameter OUTPUT_CLOCK_FREQUENCY5 = "0 MHz";
  parameter PHASE_SHIFT5 = "0 ps";
  parameter DUTY_CYCLE5 = 50;
  parameter OUTPUT_CLOCK_FREQUENCY6 = "0 MHz";
  parameter PHASE_SHIFT6 = "0 ps";
  parameter DUTY_CYCLE6 = 50;
  parameter OUTPUT_CLOCK_FREQUENCY7 = "0 MHz";
  parameter PHASE_SHIFT7 = "0 ps";
  parameter DUTY_CYCLE7 = 50;

  altera_pll #(
    .fractional_vco_multiplier("false"),
    .reference_clock_frequency(REFERENCE_CLOCK_FREQUENCY),
    .operation_mode("direct"),
    .number_of_clocks(NUMBER_OF_CLOCKS),
    .output_clock_frequency0(OUTPUT_CLOCK_FREQUENCY0),
    .phase_shift0(PHASE_SHIFT0),
    .duty_cycle0(DUTY_CYCLE0),
    .output_clock_frequency1(OUTPUT_CLOCK_FREQUENCY1),
    .phase_shift1(PHASE_SHIFT1),
    .duty_cycle1(DUTY_CYCLE1),
    .output_clock_frequency2(OUTPUT_CLOCK_FREQUENCY2),
    .phase_shift2(PHASE_SHIFT2),
    .duty_cycle2(DUTY_CYCLE2),
    .output_clock_frequency3(OUTPUT_CLOCK_FREQUENCY3),
    .phase_shift3(PHASE_SHIFT3),
    .duty_cycle3(DUTY_CYCLE3),
    .output_clock_frequency4(OUTPUT_CLOCK_FREQUENCY4),
    .phase_shift4(PHASE_SHIFT4),
    .duty_cycle4(DUTY_CYCLE4),
    .output_clock_frequency5(OUTPUT_CLOCK_FREQUENCY5),
    .phase_shift5(PHASE_SHIFT5),
    .duty_cycle5(DUTY_CYCLE5),
    .output_clock_frequency6(OUTPUT_CLOCK_FREQUENCY6),
    .phase_shift6(PHASE_SHIFT6),
    .duty_cycle6(DUTY_CYCLE6),
    .output_clock_frequency7(OUTPUT_CLOCK_FREQUENCY7),
    .phase_shift7(PHASE_SHIFT7),
    .duty_cycle7(DUTY_CYCLE7),
    .output_clock_frequency8("0 MHz"),
    .phase_shift8("0 ps"),
    .duty_cycle8(50),
    .output_clock_frequency9("0 MHz"),
    .phase_shift9("0 ps"),
    .duty_cycle9(50),
    .output_clock_frequency10("0 MHz"),
    .phase_shift10("0 ps"),
    .duty_cycle10(50),
    .output_clock_frequency11("0 MHz"),
    .phase_shift11("0 ps"),
    .duty_cycle11(50),
    .output_clock_frequency12("0 MHz"),
    .phase_shift12("0 ps"),
    .duty_cycle12(50),
    .output_clock_frequency13("0 MHz"),
    .phase_shift13("0 ps"),
    .duty_cycle13(50),
    .output_clock_frequency14("0 MHz"),
    .phase_shift14("0 ps"),
    .duty_cycle14(50),
    .output_clock_frequency15("0 MHz"),
    .phase_shift15("0 ps"),
    .duty_cycle15(50),
    .output_clock_frequency16("0 MHz"),
    .phase_shift16("0 ps"),
    .duty_cycle16(50),
    .output_clock_frequency17("0 MHz"),
    .phase_shift17("0 ps"),
    .duty_cycle17(50),
    .pll_type("General"),
    .pll_subtype("General")
  ) altera_pll_i (
    .rst(i_rst),
    .outclk({o_clk7, o_clk6, o_clk5, o_clk4, o_clk3, o_clk2, o_clk1, o_clk0}),
    .locked(o_locked),
    .fboutclk( ),
    .fbclk(1'b0),
    .refclk(i_refclk)
  );
endmodule

