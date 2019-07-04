# MC1

This is a hobby SoC computer intended for FPGA:s, based on the [MRISC32](https://github.com/mbitsnbites/mrisc32) soft microprocessor.

## Architecture

*This is work in progress*

The artchitecture is inspired by graphics oriented computers from the 1980s, such as the [Amiga](https://en.wikipedia.org/wiki/Amiga), and the goal is to make a simple computer that is fun to program.

The CPU is a full MRISC32, with support for floating point and vector operations.

Additionally there is video/VGA logic that is capable of displaying color graphics.

The CPU and the video logic has access to the same internal RAM ([FPGA block RAM](https://www.nandland.com/articles/block-ram-in-fpga.html)) - each with its own dedicated memory port.

## Planned features

* CPU:
  * A single MRISC32 core.
  * Interrupt signals from the video logic (e.g. VSYNC), once interrupt logic has been added to the MRISC32.
* Video:
  * On-chip framebuffer (size is limited by the available device BRAM).
  * 8-bit palette graphics (256 color palette, each with 24-bit RGB color resolution + 8-bit alpha).
    * Possibly other modes too (e.g. 1-bit and 4-bit) to enable higher resolutions with limited memory.
  * Two image planes (the top layer is alpha-blended on top of the bottom layer).
  * 1280x720 (HD) native resolution (compile-time configurable), with programmable lower virtual resolutions (e.g. 320x180).
  * A simple programmable raster-synchronized video controller that enables raster effects, e.g:
    * Per-line color palette updates (e.g. for [raster bars](https://en.wikipedia.org/wiki/Raster_bar)).
    * Per-line image memory location updates (can be used for controlling the vertical resolution, or for more funky dynamic effects).
    * Per-line resolution and video mode control.
* Audio:
  * Some sort of high quality audio DMA with a delta-sigma DAC.
* Memory:
  * On-chip BRAM is used for all time critical / real time RAM duties (including the video framebuffer).
  * On-chip ROM (BRAM) is used for the boot program (initially the entire program will reside here).
  * Support for off-chip RAM (e.g. DRAM or SRAM) may be added later - perhaps with an on-chip L2 cache.
* I/O:
  * Initially the only I/O will be the VGA port (video output).
  * For debugging/control, simple FPGA board I/O such as buttons and leds may be memory mapped into the CPU address space.
  * In the future, a Micro SD interface may be added to read programs and data, and perhaps an interface for mouse/keyboard (e.g. PS/2).
* Operating system:
  * Not really - perhaps a library of helper routines that can be linked to your program (e.g. for I/O and simple memory allocation routines).