/* -*- mode: ld-script; tab-width: 4; indent-tabs-mode: nil; -*-             */
/* ------------------------------------------------------------------------- */
/* Linker script for the MC1 ROM.                                            */
/* ------------------------------------------------------------------------- */

OUTPUT_FORMAT("elf32-mrisc32")
OUTPUT_ARCH("mrisc32")
ENTRY(_start)

__rom_start  = 0x00000200;
__vram_start = 0x40000100;  /* Leave room for video "registers" */

SECTIONS
{
    /* --------------------------------------------------------------------- */
    /* Read-only stuff goes into the ROM.                                    */
    /* --------------------------------------------------------------------- */

    . = __rom_start;

    .text :
    {
        *(.entry*)  /* Force the entry point to the start of the ROM */
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }


    /* --------------------------------------------------------------------- */
    /* TODO(m): The .data sections should be r/w.                            */
    /* For now, we just place them in the ROM.                               */
    /* --------------------------------------------------------------------- */

    .data :
    {
        *(.data*)
    }

    .sdata :
    {
        *(.sdata*)
    }


    /* --------------------------------------------------------------------- */
    /* BSS goes into VRAM.                                                   */
    /* We define __bss_start and __bss_size so the startup code knows what   */
    /* memory area to clear.                                                 */
    /* --------------------------------------------------------------------- */

    . = __vram_start;
    __bss_start = .;

    .sbss (NOLOAD) :
    {
        *(.sbss*)
        *(.scommon*)
    }

    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    }

    . = ALIGN(4);
    __bss_size = . - __bss_start;


    /* This tells the system where it can start to allocate VRAM. */
    __vram_free_start = .;
}
