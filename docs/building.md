# Building the MC1

These build instructions assume that you are running some flavor of Linux.

## Prerequisites

Start by cloning this repository, and update all submodules:

```bash
git submodule update --init --recursive
```

You also need a working installation of the [MRISC32 GNU toolchain](https://github.com/mrisc32/mrisc32-gnu-toolchain) (in your PATH).

## Build the ROM

First build the MC1 SDK tools, according to the instructions in the tools README (`src/mc1-sdk/tools/README.md`).

The ROM source code is located in [src/rom](../src/rom/). When building the ROM, a VHDL file is created that is used when building the VHDL design.

```bash
cd src/rom
make -j20
```

If a change is made to the ROM source code, this step needs to be repeated before re-building the VHDL design.

## Synthesizing the VHDL design

To synthesize the design for a target FPGA you need:

* A tool that understands VHDL 2008.
* A toplevel design file, including:
  * Interfaces for things like LED:s, SD-card and keyboard.
  * The MC1 & MRISC32-A1 configuration.
* Optionally device specific entities, e.g. PLL:s.

Currently, toplevel designs are provided for the following boards:

* Terasic DE0-CV (Cyclone V 5CEBA4F23C7N).
* Terasic DE10-Lite (MAX 10 10M50DAF484C7G).

### Intel Quartus (DE0-CV, DE10-Lite)

Note: This has been tested with Quartus 19.1.

#### Create project

Create a new empty project in Quartus using the Project Wizard.

Add the following files to the project:

* All files in **`src/rtl`**
  * Except: ~~`ram_true_dual_port.vhd`~~
* All files in **`src/rtl/de0_cv`** *or* **`src/rtl/de10_lite`**
* All files in the subfolders of **`src/mrisc32-a1/rtl`**
  * Except: ~~`toplevel.vhd`~~, ~~`fpu/fpu_test_gen.cpp`~~
* **`src/rom/out/rom.vhd`**

Select the right FPGA device for your board.

When the project has been created, make the following project settings:

* General: Top-level entity: **toplevel**
* Files: Mark all `mrisc32-a1/*` files, click Properties, enter **mrisc32** in the Library field, press OK, and Apply.
* Compiler Settings: Optimization mode = **Performance (aggressive)**
* Compiler Settings > VHDL Input: VHDL version = **VHDL 2008**

#### Compile & program

* Compile Design
* Program Device
