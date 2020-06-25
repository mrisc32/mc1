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
        *(.text.entry)
        *(.text.start)
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

    /* C++ code may generate an eh_frame section. We don't use exceptions, but
       just in case it slips in, we put eh_frame sections in the ROM just to
       avoid having it end up in RAM (which would confuse objcopy into
       producing a multi-GB ROM image). */
    .eh_frame :
    {
        *(.eh_frame*)
    }

    __rom_size = . - 0x00000000;


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
