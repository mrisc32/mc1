## Video control program

A video control program (VCP) is a sequence of video control commands.

The only way to alter the color palette or any of the video control registers
is via a VCP.

The program is started during the vertical blanking interval, and continues to
exectue until the next vertical blanking interval.

To mark the end of a program, issue a `WAIT` command that waits for a line
that will never be displayed (e.g. `WAIT 32767`).


## Video control commands

Each video control command (VCC) is 32 bits wide.

The two most significant bits give the command, according to:

| Code (bin) | Command | Description                                 |
|------------|---------|---------------------------------------------|
| 00         | JUMP    | Program flow control (NOP, JMP, JSR, RTS)   |
| 01         | WAIT    | Wait until the given raster line is reached |
| 10         | SETREG  | Set the value of a video control register   |
| 11         | SETPAL  | Set the palette                             |

### JUMP

The JUMP command is in fact one of four sub-commands: NOP, JMP, JSR or RTS.

#### NOP

The NOP command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 29-26 | (unused)               |
| 25-24 | 00                     |
|  27-0 | (unused)               |

The NOP instruction does nothing (except advancing the program pointer to the next instruction).

#### JMP

The JMP command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 29-26 | (unused)               |
| 25-24 | 01                     |
|  23-0 | Target address         |

The JMP jumps to the given target address (without affecting the internal call stack).

#### JSR

The JSR command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 29-26 | (unused)               |
| 25-24 | 10                     |
|  23-0 | Target address         |

The JSR command pushes the address to the next instruction onto the internal call stack, and then jumps to the given target address.

Note: The internal call stack is implemented as a circular buffer with 16 entries.

#### RTS

The RTS command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 29-26 | (unused)               |
| 25-24 | 11                     |
| 23-0  | (unused)               |

The RTS command pops an instruction address from the top of the internal call stack, and jumps to that address.

Note: Since the stack is never reset, issuing an RTS command without a matching JSR command will result in undefined behaviour.

### WAIT

The WAIT command is encoded as follows:

| Bits  | Description                        |
|-------|------------------------------------|
| 29-16 | (unused)                           |
|  15-0 | Raster line number (-32768..32767) |

### SETREG

The SETREG command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 29-24 | Register number (0-63) |
|  23-0 | 24-bit value           |

### SETPAL

The SETPAL command is encoded as follows:

| Bits  | Description                      |
|-------|----------------------------------|
| 29-16 | (unused)                         |
|  15-8 | First palette entry (0-255)      |
|   7-0 | Number of entries - 1, N (0-255) |

After the SETPAL command, N+1 number of 32-bit RGBA color values follow in the VCP stream.


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

