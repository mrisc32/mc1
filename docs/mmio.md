# Memory mapped I/O

There are a number of memory mapped input and output registers that can be read and written by the CPU.

The registers are all 32 bits wide, and are located in the I/O memory area.

| Reg | Name | R/W | Description |
|-----|------|-----|-------------|
|  0  | CLKCNTLO |  R  | CPU clock cycle count  (free running counter) - lo bits |
|  1  | CLKCNTHI |  R  | CPU clock cycle count - hi bits |
|  2  | CPUCLK |  R  | CPU clock frequency in Hz |
|  3  | VRAMSIZE |  R  | VRAM size in bytes |
|  4  | XRAMSIZE |  R  | Extended RAM size in bytes |
|  5  | VIDWIDTH |  R  | Native video resoltuion, width |
|  6  | VIDHEIGHT |  R  | Native video resoltuion, height |
|  7  | VIDFPS |  R  | Video refresh rate in 65536 * frames per s |
|  8  | VIDFRAMENO |  R  | Video frame number (free running counter) |
|  9  | VIDX |  R  | Video raster X position |
| 10  | VIDY |  R  | Video raster Y position |
| 11  | SWITCHES |  R  | Switches (one bit per switch, active high) |
| 12  | BUTTONS |  R  | Buttons (one bit per switch, active high) |
| 16  | SEGDISP0 |  R/W  | Segmented display 0 (one bit per segment, active high) |
| 17  | SEGDISP1 |  R/W  | Segmented display 1 (one bit per segment, active high) |
| 18  | SEGDISP2 |  R/W  | Segmented display 2 (one bit per segment, active high) |
| 19  | SEGDISP3 |  R/W  | Segmented display 3 (one bit per segment, active high) |
| 20  | SEGDISP4 | R/W  | Segmented display 4 (one bit per segment, active high) |
| 21  | SEGDISP5 | R/W  | Segmented display 5 (one bit per segment, active high) |
| 22  | SEGDISP6 | R/W  | Segmented display 6 (one bit per segment, active high) |
| 23  | SEGDISP7 | R/W  | Segmented display 7 (one bit per segment, active high) |
| 24  | LEDS | R/W | LED:s (one bit per LED, active high) |
