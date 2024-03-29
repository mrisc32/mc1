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
        KEEP (*(SORT_NONE(.init)))
        KEEP (*(SORT_NONE(.fini)))
    }
    . = ALIGN(4);

    .rodata :
    {
        *(.rodata*)
    }
    . = ALIGN(4);


    /* --------------------------------------------------------------------- */
    /* C++ helpers (ctor/dtor/eh_frame) go into the ROM.                     */
    /* --------------------------------------------------------------------- */

    .ctor :
    {
        __CTOR_START = .;
        KEEP (*crtbegin.o(.ctors))
        KEEP (*crtbegin?.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        __CTOR_END = .;
    }
    . = ALIGN(4);

    .dtor :
    {
        __DTOR_START = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*crtbegin?.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        __DTOR_END = .;
    }
    . = ALIGN(4);

    .eh_frame :
    {
        *(.eh_frame*)
    }
    . = ALIGN(4);


    /* --------------------------------------------------------------------- */
    /* TODO(m): The .data sections should be r/w.                            */
    /* For now, we just place them in the ROM.                               */
    /* --------------------------------------------------------------------- */

    .data :
    {
        *(.data*)
    }
    . = ALIGN(4);

    .sdata :
    {
        *(.sdata*)
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
    . = ALIGN(4);

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
