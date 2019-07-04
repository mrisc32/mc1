## Video control registers

Each register is 32 bits wide.

| Reg No.  | Hex   | Descr.                                                            |
|----------|-------|-------------------------------------------------------------------|
| 0-255    | 0x000 | Layer 0 palette                                                   |
| 256      | 0x100 | Layer 0 video memory start address                                |
| 257      | 0x101 | Layer 0 X resolution (1, 2, 3, 4, ...)                            |
| 258      | 0x102 | Layer 0 X start position                                          |
| 259      | 0x103 | Layer 0 X stop position                                           |
| 260      | 0x104 | Layer 0 color mode (0 = 8 bpp, 1 = 4 bpp, 2 = 2 bpp, 3 = 1 bpp)   |
| 512-767  | 0x200 | Layer 1 palette                                                   |
| 768      | 0x300 | Layer 1 video memory start address                                |
| 769      | 0x301 | Layer 1 X resolution (1, 2, 3, 4, ...)                            |
| 770      | 0x302 | Layer 1 X start position                                          |
| 771      | 0x304 | Layer 1 X stop position                                           |
| 772      | 0x305 | Layer 1 color mode (0 = 8 bpp, 1 = 4 bpp, 2 = 2 bpp, 3 = 1 bpp)   |
