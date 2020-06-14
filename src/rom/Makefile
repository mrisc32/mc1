# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-
#--------------------------------------------------------------------------------------------------
# Copyright (c) 2019 Marcus Geelnard
#
# This software is provided 'as-is', without any express or implied warranty. In no event will the
# authors be held liable for any damages arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose, including commercial
# applications, and to alter it and redistribute it freely, subject to the following restrictions:
#
#  1. The origin of this software must not be misrepresented; you must not claim that you wrote
#     the original software. If you use this software in a product, an acknowledgment in the
#     product documentation would be appreciated but is not required.
#
#  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
#     being the original software.
#
#  3. This notice may not be removed or altered from any source distribution.
#--------------------------------------------------------------------------------------------------

OUT = out
SELFTESTOUT = selftest/out

HOST_CC      = gcc
HOST_CCFLAGS = -c -O2 -Wall -Wextra -Werror -MMD -MP

CC      = mrisc32-elf-gcc
CCFLAGS = -c -Os -I . -ffast-math -Wall -Wextra -Werror -MMD -MP
AS      = mrisc32-elf-gcc
ASFLAGS = -c -I .
LD      = mrisc32-elf-gcc
LDFLAGS = -L$(OUT) -L$(SELFTESTOUT) -T link.ld -mno-crt0
AR      = mrisc32-elf-ar
ARFLAGS = rcs
OBJCOPY = mrisc32-elf-objcopy
CP      = cp -a

.PHONY: clean all selftest

all: $(OUT)/rom.vhd $(OUT)/png2mci

clean:
	rm -f $(OUT)/*.a \
	      $(OUT)/*.d \
	      $(OUT)/*.s \
	      $(OUT)/*.o \
	      $(OUT)/*.elf \
	      $(OUT)/*.raw \
	      $(OUT)/*.vhd
	$(MAKE) -C selftest clean


#-----------------------------------------------------------------------------
# Tools (compiled for the host!)
#-----------------------------------------------------------------------------

PNG2MCI_OBJS = \
    $(OUT)/png2mci.o \
    $(OUT)/lodepng.o

$(OUT)/png2mci: $(PNG2MCI_OBJS)
	$(HOST_CC) -o $@ $(PNG2MCI_OBJS)

$(OUT)/png2mci.o: tools/png2mci.c
	$(HOST_CC) $(HOST_CCFLAGS) -o $@ $<

$(OUT)/lodepng.o: tools/lodepng/lodepng.c
	$(HOST_CC) $(HOST_CCFLAGS) -o $@ $<


#-----------------------------------------------------------------------------
# Demo application
#-----------------------------------------------------------------------------

DEMO_OBJS = \
    $(OUT)/demo.o \
    $(OUT)/mandelbrot.o \
    $(OUT)/raytrace.o \
    $(OUT)/retro.o

$(OUT)/demo.o: demo/demo.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/mandelbrot.o: demo/mandelbrot.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/raytrace.o: demo/raytrace.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/retro.o: demo/retro.c
	$(CC) $(CCFLAGS) -o $@ $<


#-----------------------------------------------------------------------------
# ROM image
#-----------------------------------------------------------------------------

$(OUT)/rom.vhd: $(OUT)/rom.raw rom.vhd.in
	tools/raw2vhd.py $(OUT)/rom.raw rom.vhd.in > $@

$(OUT)/rom.raw: $(OUT)/rom.elf
	$(OBJCOPY) -O binary $< $@

$(OUT)/rom.elf: $(DEMO_OBJS) $(OUT)/libmc1.a $(OUT)/libselftest.a link.ld
	$(LD) $(LDFLAGS) -o $@ $(DEMO_OBJS) -lmc1 -lselftest -lm


#-----------------------------------------------------------------------------
# libselftest.a - Selftest library
#-----------------------------------------------------------------------------

$(OUT)/libselftest.a: selftest
	@$(CP) $(SELFTESTOUT)/libselftest.a $(OUT)/libselftest.a

selftest:
	$(MAKE) -C selftest


#-----------------------------------------------------------------------------
# libmc1.a - MC1 system library
#-----------------------------------------------------------------------------

LIBMC1_OBJS = \
    $(OUT)/crt0.o \
    $(OUT)/fast_math.o \
    $(OUT)/framebuffer.o \
    $(OUT)/leds.o \
    $(OUT)/mc1-font-8x8.o \
    $(OUT)/mem_copy.o \
    $(OUT)/mem_fill.o \
    $(OUT)/memory.o \
    $(OUT)/time.o \
    $(OUT)/vconsole.o \
    $(OUT)/vcp.o

$(OUT)/libmc1.a: $(LIBMC1_OBJS)
	$(AR) $(ARFLAGS) $@ $(LIBMC1_OBJS)

$(OUT)/crt0.o: mc1/crt0.s mc1/memory.inc mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ mc1/crt0.s

$(OUT)/fast_math.o: mc1/fast_math.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/framebuffer.o: mc1/framebuffer.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/leds.o: mc1/leds.s mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ mc1/leds.s

$(OUT)/mc1-font-8x8.o: $(OUT)/mc1-font-8x8.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/mc1-font-8x8.s: mc1/mc1-font-8x8.raw
	# Trim out ASCII code points 32..127 (the printable ASCII chars).
	cat mc1/mc1-font-8x8.raw | \
	    tail -c +257  | \
	    head -c -1024 \
	    > $(OUT)/mc1-font-8x8-part.raw
	tools/raw2asm.py --rev $(OUT)/mc1-font-8x8-part.raw mc1_font_8x8 > $@

$(OUT)/mem_copy.o: mc1/mem_copy.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/mem_fill.o: mc1/mem_fill.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/memory.o: mc1/memory.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/time.o: mc1/time.s mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/vconsole.o: mc1/vconsole.s mc1/mmio.inc mc1/memory.inc
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/vcp.o: mc1/vcp.c
	$(CC) $(CCFLAGS) -o $@ $<


# Include dependency files (generated when building the object files).
-include $(PNG2MCI_OBJS:.o=.d)
-include $(DEMO_OBJS:.o=.d)
-include $(LIBMC1_OBJS:.o=.d)

