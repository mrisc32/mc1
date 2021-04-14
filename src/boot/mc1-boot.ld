/* -*- mode: ld-script; tab-width: 4; indent-tabs-mode: nil; -*-             */
/* ------------------------------------------------------------------------- */
/* Copyright (c) 2021 Marcus Geelnard                                        */
/*                                                                           */
/* This software is provided 'as-is', without any express or implied         */
/* warranty. In no event will the authors be held liable for any damages     */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/*  1. The origin of this software must not be misrepresented; you must not  */
/*     claim that you wrote the original software. If you use this software  */
/*     in a product, an acknowledgment in the product documentation would be */
/*     appreciated but is not required.                                      */
/*                                                                           */
/*  2. Altered source versions must be plainly marked as such, and must not  */
/*     be misrepresented as being the original software.                     */
/*                                                                           */
/*  3. This notice may not be removed or altered from any source             */
/*     distribution.                                                         */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Linker script for MC1 boot blocks.                                        */
/* ------------------------------------------------------------------------- */

OUTPUT_FORMAT("elf32-mrisc32")
OUTPUT_ARCH("mrisc32")
ENTRY(_boot)

__boot_start = 0x40010000;  /* DOES NOT MATTER - CODE MUST BE PC RELATIVE! */
__vram_start = 0x40000100;  /* Leave room for video "registers" */

SECTIONS
{
    /* --------------------------------------------------------------------- */
    /* Put code and data after each other in the boot block.                 */
    /* --------------------------------------------------------------------- */

    . = __boot_start;

    .text :
    {
        *(.text.start)  /* Ensure that the _boot() function is placed first */
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data*)
    }

    .sdata :
    {
        *(.sdata*)
    }

    __boot_size = . - __boot_start;


    /* --------------------------------------------------------------------- */
    /* BSS goes into VRAM.                                                   */
    /* We define __bss_start and __bss_size so the startup code knows what   */
    /* memory area to clear.                                                 */
    /*                                                                       */
    /* Note: Typically boot code will no use BSS memory (and it would be     */
    /* responsible for clearing BSS itself), but we provide support for it   */
    /* anyway, for completeness.                                             */
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
