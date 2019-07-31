## Video control program

A video control program (VCP) is a sequence of video control commands.

The only way to alter the color palette or any of the video control registers
is via a VCP.

The program is started during the vertical blanking interval, and continues to
exectue until the next vertical blanking interval.

To mark the end of a program, issue a `WAIT` command that waits for a line
that will never be displayed (e.g. `WAIT 65535`).


## Video control commands

Each video control command (VCC) is 32 bits wide.

The two most significant bits give the command, according to:

| Code (bin) | Command | Description                                 |
|------------|---------|---------------------------------------------|
| 00         | WAIT    | Wait until the given raster line is reached |
| 01         | SETREG  | Set the value of a video control register   |
| 10         | SETPAL  | Set the palette                             |
| 11         | -       | (reserved)                                  |

### WAIT

The WAIT command is encoded as follows:

| Bits  | Description        |
|-------|--------------------|
| 29-16 | (unused)           |
|  15-0 | Raster line number |

### SETREG

The SETREG command is encoded as follows:

| Bits  | Description            |
|-------|------------------------|
| 29-24 | Register number (0-15) |
|  23-0 | 24-bit value           |

### SETPAL

The SETPAL command is encoded as follows:

| Bits  | Description                  |
|-------|------------------------------|
| 29-16 | (unused)                     |
|  15-8 | First palette entry (0-255)  |
|   7-0 | Number of entries, N (0-255) |

After the SETPAL command, N number of 32-bit RGBA color values follow in the VCP stream.


## Video control registers

Each video control register (VCR) is 24 bits wide.

| Reg | Name | Description |
|-----|------|-------------|
| 0   | ADDR | Row start address (word address = byte address / 4)<br>Default: 0x000000 |
| 1   | XSTRT | X start coordinate (unsigned fixed point, 8.16 bits)<br>Default: 0x000000 |
| 2   | XINCR | X coordinate increment (unsigned fixed point, 8.16 bits)<br>Default: 0x004000 (0.25) |
| 3   | HSTRT | Horizontal screen start position<br>Default: 0 |
| 4   | HSTOP | Horizontal screen stop position<br>Default: 0 |
| 5   | CMODE | Color mode:<br>0 = 32 bpp<br>1 = 16 bpp<br>2 = 8 bpp (default)<br>3 = 4 bpp<br>4 = 2 bpp<br>5 = 1 bpp |
