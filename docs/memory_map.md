# MC1 memory map

## CPU address spaces

The 32-bit CPU address space is divided into four sections, each 1 GiB in size.

| Address | Description |
| --- | --- |
| 00000000 - 3FFFFFFF | ROM |
| 40000000 - 7FFFFFFF | VRAM |
| 80000000 - BFFFFFFF | XRAM |
| C0000000 - FFFFFFFF | MMIO |

**NOTE:** In most implementations, the physically available ROM, VRAM and XRAM is much less than 1 GiB, in which case each memory area will contain multiple mirrors of the physical memory. E.g. if an MC1 implementation has 256 KiB of VRAM, there will be 1048576 / 256 = 4096 mirrors in the VRAM address range.

## Video logic addressing

The video logic addressing is slightly different than that of the CPU:

  * Only VRAM can be accessed.
  * The smallest addressable unit is a 32-bit word.
  * An address is 24 bits wide (000000 - FFFFFF), representing 2<sup>24</sup> 32-bit words (or 64 MiB), where address 000000 maps to CPU address 40000000.

| Video logic address | CPU address |
| --- | --- |
| 000000 | 40000000 |
| 000001 | 40000004 |
| 000002 | 40000008 |
| ... | ... |
| FFFFFF | 43FFFFFC |

## ROM

| Address | Description |
| --- | --- |
| 00000000 - 000001FF | (unused, zero) |
| 00000200 | CPU reset/boot start address |

## VRAM (video RAM)

The video RAM can be accessed by both the CPU and the video logic hardware, and as such may hold Video Control Programs and pixel data. The CPU can read or write one 32-bit word from/to VRAM on every clock cycle.

| Address | Description |
| --- | --- |
| 40000010 | Video layer 1 VCP start address |
| 40000020 | Video layer 2 VCP start address |
| 40000100 | BSS start for the ROM code |

The size of the VRAM can be queried via the VRAMSIZE MMIO register.

## XRAM (extended RAM)

While VRAM is mandatory for every MC1 implementation, XRAM is optional (i.e. the XRAM size may be zero).

The XRAM typically has longer latencies than the VRAM, as it may be implemented by off chip dynamic RAM (e.g. SDRAM or DDR3).

The presence and the size of the XRAM can be queried via the XRAMSIZE MMIO register.

## MMIO (memory mapped I/O)

There are a number of memory mapped input and output registers that can be accessed by the CPU.

The registers are all 32 bits wide, and are located in the I/O memory area starting at C0000000.

| Address | Name | R/W | Description |
| --- | --- | --- | --- |
| C0000000 | CLKCNTLO |  R  | CPU clock cycle count  (free running counter) - lo bits |
| C0000004 | CLKCNTHI |  R  | CPU clock cycle count - hi bits |
| C0000008 | CPUCLK |  R  | CPU clock frequency in Hz |
| C000000C | VRAMSIZE |  R  | VRAM size in bytes |
| C0000010 | XRAMSIZE |  R  | Extended RAM size in bytes |
| C0000014 | VIDWIDTH |  R  | Native video resoltuion, width |
| C0000018 | VIDHEIGHT |  R  | Native video resoltuion, height |
| C000001C | VIDFPS |  R  | Video refresh rate, in 65536 * frames per s |
| C0000020 | VIDFRAMENO |  R  | Video frame number (free running counter) |
| C0000024 | VIDY |  R  | Video raster Y position (signed) |
| C0000028 | SWITCHES |  R  | Switches (one bit per switch, active high) |
| C000002C | BUTTONS |  R  | Buttons (one bit per switch, active high) |
| C0000030 | KEYPTR |  R  | Key event buffer pointer (index in bits 0..3, wrap count in bits 4..31) |
| C0000034 | MOUSEPOS |  R  | Mouse pos (x in bits 0..15, y in bits 16..31) |
| C0000038 | MOUSEBTNS |  R  | Mouse buttons (bit 0 = left, bit 1 = middle, bit 2 = right, ...) |
| C000003C | SDIN |  R  | SD card input (bit 0 = DAT0/MISO, bit 1 = DAT1, bit 2 = DAT2, bit 3 = DAT3/SS*, bit 4 = CMD/MOSI) |
| C0000040 | SEGDISP0 |  R/W  | Segmented display 0 (one bit per segment, active high) |
| C0000044 | SEGDISP1 |  R/W  | Segmented display 1 (one bit per segment, active high) |
| C0000048 | SEGDISP2 |  R/W  | Segmented display 2 (one bit per segment, active high) |
| C000004C | SEGDISP3 |  R/W  | Segmented display 3 (one bit per segment, active high) |
| C0000050 | SEGDISP4 | R/W  | Segmented display 4 (one bit per segment, active high) |
| C0000054 | SEGDISP5 | R/W  | Segmented display 5 (one bit per segment, active high) |
| C0000058 | SEGDISP6 | R/W  | Segmented display 6 (one bit per segment, active high) |
| C000005C | SEGDISP7 | R/W  | Segmented display 7 (one bit per segment, active high) |
| C0000060 | LEDS | R/W | LED:s (one bit per LED, active high) |
| C0000064 | SDOUT | R/W | SD card output (bit 0 = DAT0/MISO, bit 1 = DAT1, bit 2 = DAT2, bit 3 = DAT3/SS*, bit 4 = CMD/MOSI, bit 5 = CLK/SCK) |
| C0000068 | SDWE | R/W | SD card write enable bit mask for bits 0-4 of SDOUT |
| C000006C | - |  -  | (reserved) |
| C0000070 | - |  -  | (reserved) |
| C0000074 | - |  -  | (reserved) |
| C0000078 | - |  -  | (reserved) |
| C000007C | - |  -  | (reserved) |
| C0000080 | KEYBUF |  R  | Key event buffer (16 32-bit words) |

### Key event buffer

The key event buffer is a circualt FIFO buffer. Each entry is encoded as follows:

* Bits 0..9: Key code
* Bit 31: 1=press, 0=release

The `KEYPTR` is incremented by 1 every time a key event is added to the buffer, and the four least significant bits give the index of the latest event in the `KEYBUF` array, which is 16 entries long.

