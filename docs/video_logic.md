# Video logic

## Video control program

A video control program (VCP) is a sequence of video control commands. See
the [VCP examples](../tools/vcpas/examples) for what a VCP can look like.

The only way to alter the color palette or any of the video control registers
is via a VCP.

The program is started during the vertical blanking interval, and continues to
exectue until the next vertical blanking interval.

To mark the end of a program, issue a `WAIT` command that waits for a line
that will never be displayed (e.g. `WAIT 32767`).


## Video control commands

Each video control command (VCC) is 32 bits wide.

The four most significant bits give the command, according to:

| Code (bin) | Command | Description                                     |
|------------|---------|-------------------------------------------------|
| 0000       | NOP     | No operation                                    |
| 0001       | JMP     | Jump to a target address                        |
| 0010       | JSR     | Jump to a subroutine (push return address)      |
| 0011       | RTS     | Return from a subroutine (pop return address)   |
| 0100       | WAITX   | Wait until the given raster column is reached   |
| 0101       | WAITY   | Wait until the given raster row is reached      |
| 0110       | SETPAL  | Set the palette                                 |
| 1000       | SETREG  | Set the value of a video control register       |

### NOP

The NOP command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 31-28 | 0000                   |
|  27-0 | (unused)               |

The NOP instruction does nothing (except advancing the program pointer to the next instruction).

### JMP

The JMP command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 31-28 | 0001                   |
| 27-24 | (unused)               |
|  23-0 | Target address         |

The JMP jumps to the given target address (without affecting the internal call stack).

### JSR

The JSR command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 31-28 | 0010                   |
| 27-24 | (unused)               |
|  23-0 | Target address         |

The JSR command pushes the address to the next instruction onto the internal call stack, and then jumps to the given target address.

Note: The internal call stack is implemented as a circular buffer with 16 entries.

### RTS

The RTS command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 31-28 | 0011                   |
|  27-0 | (unused)               |

The RTS command pops an instruction address from the top of the internal call stack, and jumps to that address.

Note: Since the stack is never reset, issuing an RTS command without a matching JSR command will result in undefined behaviour.

### WAITX

The WAITX command is encoded as follows:

| Bits  | Description                          |
|-------|--------------------------------------|
| 31-28 | 0100                                 |
| 27-16 | (unused)                             |
|  15-0 | Raster column number (-32768..32767) |

The command waits until the specified column is reached. If the specified column has already passed, the command will wait for the same column in the next raster line.

### WAITY

The WAITY command is encoded as follows:

| Bits  | Description                       |
|-------|-----------------------------------|
| 31-28 | 0101                              |
| 27-16 | (unused)                          |
|  15-0 | Raster row number (-32768..32767) |

The command waits until the specified row is reached. If the specified row has already passed, the command will wait until the end of the frame.

### SETPAL

The SETPAL command is encoded as follows:

| Bits  | Description                      |
|-------|----------------------------------|
| 31-28 | 0110                             |
| 27-16 | (unused)                         |
|  15-8 | First palette entry (0-255)      |
|   7-0 | Number of entries - 1, N (0-255) |

After the SETPAL command, N+1 number of RGBA8888 (ABGR32) color values follow in the VCP stream.

### SETREG

The SETREG command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 31-28 | 1000                   |
| 27-24 | Register number (0-15) |
|  23-0 | 24-bit value           |

The SETREG commands sets the given register (VCR) to the specified 24-bit value.

## Video control registers

Each video control register (VCR) is 24 bits wide.

| Reg | Name | Description |
|-----|------|-------------|
| 0   | ADDR | Row start address (word address = byte address / 4)<br>Default: 0x000000 |
| 1   | XOFFS | X coordinate offset (signed fixed point, 8.16 bits)<br>Default: 0x000000 |
| 2   | XINCR | X coordinate increment (signed fixed point, 8.16 bits)<br>Default: 0x004000 (0.25) |
| 3   | HSTRT | Horizontal screen start position<br>Default: 0 |
| 4   | HSTOP | Horizontal screen stop position<br>Default: 0 |
| 5   | CMODE | Color mode:<br>0 = RGBA8888 (32 bpp)<br>1 = RGBA5551 (16 bpp)<br>2 = PAL8 (8 bpp, default)<br>3 = PAL4 (4 bpp)<br>4 = PAL2 (2 bpp)<br>5 = PAL1 (1 bpp) |
| 6   | RMODE | Bits 0-1: Dither method:<br>0 = no dithering (default)<br>1 = white noise dithering |

## Pixel pipeline

The pixel pipeline uses the configuration given by the video control registers to read data from VRAM and convert it to 24-bit RGB pixel values.

## Dithering

As a final step the 24-bit RGB color is dithered to the resolution that is supported by the target hardware. For instance if the video output of a device is 12-bit VGA (4 bits per color component), the color will be dithered from 24 bits to 12 bits.

The dithering method is selected via the `RMODE` register.