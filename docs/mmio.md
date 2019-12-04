# Memory mapped I/O

There are a number of memory mapped input and output registers that can be read and written by the CPU.

The registers are all 32 bits wide, and are located in the I/O memory area starting at `0xc0000000`.

| Addr. | Name | R/W | Description |
|--------|------|-----|-------------|
|     0  | CLKCNTLO |  R  | CPU clock cycle count  (free running counter) - lo bits |
|     4  | CLKCNTHI |  R  | CPU clock cycle count - hi bits |
|     8  | CPUCLK |  R  | CPU clock frequency in Hz |
|    12  | VRAMSIZE |  R  | VRAM size in bytes |
|    16  | XRAMSIZE |  R  | Extended RAM size in bytes |
|    20  | VIDWIDTH |  R  | Native video resoltuion, width |
|    24  | VIDHEIGHT |  R  | Native video resoltuion, height |
|    28  | VIDFPS |  R  | Video refresh rate in 65536 * frames per s |
|    32  | VIDFRAMENO |  R  | Video frame number (free running counter) |
|    36  | VIDY |  R  | Video raster Y position |
|    40  | SWITCHES |  R  | Switches (one bit per switch, active high) |
|    44  | BUTTONS |  R  | Buttons (one bit per switch, active high) |
|    64  | SEGDISP0 |  R/W  | Segmented display 0 (one bit per segment, active high) |
|    68  | SEGDISP1 |  R/W  | Segmented display 1 (one bit per segment, active high) |
|    72  | SEGDISP2 |  R/W  | Segmented display 2 (one bit per segment, active high) |
|    76  | SEGDISP3 |  R/W  | Segmented display 3 (one bit per segment, active high) |
|    80  | SEGDISP4 | R/W  | Segmented display 4 (one bit per segment, active high) |
|    84  | SEGDISP5 | R/W  | Segmented display 5 (one bit per segment, active high) |
|    88  | SEGDISP6 | R/W  | Segmented display 6 (one bit per segment, active high) |
|    92  | SEGDISP7 | R/W  | Segmented display 7 (one bit per segment, active high) |
|    96  | LEDS | R/W | LED:s (one bit per LED, active high) |
