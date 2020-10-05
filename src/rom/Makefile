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

HOST_OUT      = $(OUT)/host
HOST_CC       = gcc
HOST_CXX      = g++
HOST_CCFLAGS  = -c -O2 -I tools/liblzg/include -Wall -Wextra -Werror -MMD -MP
HOST_CXXFLAGS = $(HOST_CCFLAGS)

CFLAGS_COMMON = -c -I . -Os -ffast-math -Wall -Wextra -Wshadow -pedantic -Werror -MMD -MP

CC       = mrisc32-elf-gcc
CCFLAGS  = $(CFLAGS_COMMON) -std=c11
CXX      = mrisc32-elf-g++
CXXFLAGS = $(CFLAGS_COMMON) -std=c++17 -Wold-style-cast -fno-exceptions
AS       = mrisc32-elf-gcc
ASFLAGS  = -c -I .
LD       = mrisc32-elf-gcc
LDFLAGS  = -L$(OUT) -L$(SELFTESTOUT) -T link.ld -mno-crt0
AR       = mrisc32-elf-ar
ARFLAGS  = rcs
OBJCOPY  = mrisc32-elf-objcopy
CP       = cp -a

DHRYSTONE_FLAGS = -c -w -fno-inline -O3

.PHONY: clean all selftest

all: $(OUT)/rom.vhd $(HOST_OUT)/png2mci $(HOST_OUT)/font_test

clean:
	rm -f $(OUT)/*.a \
	      $(OUT)/*.c \
	      $(OUT)/*.d \
	      $(OUT)/*.s \
	      $(OUT)/*.o \
	      $(OUT)/*.elf \
	      $(OUT)/*.mci \
	      $(OUT)/*.raw \
	      $(OUT)/*.vhd \
	      $(HOST_OUT)/*.o \
	      $(HOST_OUT)/*.d \
	      $(HOST_OUT)/png2mci \
	      $(HOST_OUT)/font_test
	$(MAKE) -C selftest clean


#-----------------------------------------------------------------------------
# Tools (compiled for the host!)
#-----------------------------------------------------------------------------

PNG2MCI_OBJS = \
    $(HOST_OUT)/png2mci.o \
    $(HOST_OUT)/lodepng.o \
    $(HOST_OUT)/checksum.o \
    $(HOST_OUT)/decode.o \
    $(HOST_OUT)/encode.o \
    $(HOST_OUT)/version.o

$(HOST_OUT)/png2mci: $(PNG2MCI_OBJS)
	$(HOST_CC) -o $@ $(PNG2MCI_OBJS)

$(HOST_OUT)/png2mci.o: tools/png2mci.c
	$(HOST_CC) $(HOST_CCFLAGS) -o $@ $<

$(HOST_OUT)/lodepng.o: tools/lodepng/lodepng.c
	$(HOST_CC) $(HOST_CCFLAGS) -o $@ $<

$(HOST_OUT)/checksum.o: tools/liblzg/lib/checksum.c
	$(HOST_CC) $(HOST_CCFLAGS) -w -o $@ $<

$(HOST_OUT)/decode.o: tools/liblzg/lib/decode.c
	$(HOST_CC) $(HOST_CCFLAGS) -w -o $@ $<

$(HOST_OUT)/encode.o: tools/liblzg/lib/encode.c
	$(HOST_CC) $(HOST_CCFLAGS) -w -o $@ $<

$(HOST_OUT)/version.o: tools/liblzg/lib/version.c
	$(HOST_CC) $(HOST_CCFLAGS) -w -o $@ $<

FONT_TEST_OBJS = \
    $(HOST_OUT)/font_test.o \
    $(HOST_OUT)/glyph_renderer.o

$(HOST_OUT)/font_test: $(FONT_TEST_OBJS)
	$(HOST_CXX) -o $@ $(FONT_TEST_OBJS)

$(HOST_OUT)/font_test.o: tools/font_test.cpp
	$(HOST_CXX) $(HOST_CXXFLAGS) -I . -o $@ $<

$(HOST_OUT)/glyph_renderer.o: mc1/glyph_renderer.cpp
	$(HOST_CXX) $(HOST_CXXFLAGS) -I . -o $@ $<


#-----------------------------------------------------------------------------
# Demo application
#-----------------------------------------------------------------------------

DEMO_OBJS = \
    $(OUT)/console.o \
    $(OUT)/demo.o \
    $(OUT)/mandelbrot.o \
    $(OUT)/mrisc32_logo.o \
    $(OUT)/raytrace.o \
    $(OUT)/retro.o \
    $(OUT)/stars.o

# Optional features of the demo.
DEMO_INTERACTIVE_MODE = no
DEMO_ENABLE_SELFTEST = no
DEMO_ENABLE_DHRYSTONE = no

DEMO_FLAGS =
ifeq ($(DEMO_INTERACTIVE_MODE),yes)
  DEMO_FLAGS += -DINTERACTIVE_MODE
endif
ifeq ($(DEMO_ENABLE_SELFTEST),yes)
  DEMO_FLAGS += -DENABLE_SELFTEST
endif
ifeq ($(DEMO_ENABLE_DHRYSTONE),yes)
  DEMO_FLAGS += -DENABLE_DHRYSTONE
  DEMO_OBJS += \
      $(OUT)/dhry_1.o \
      $(OUT)/dhry_2.o
endif

$(OUT)/console.o: demo/console.cpp
	$(CXX) $(CXXFLAGS) $(DEMO_FLAGS) -o $@ $<

$(OUT)/demo.o: demo/demo.c
	$(CC) $(CCFLAGS) $(DEMO_FLAGS) -o $@ $<

$(OUT)/dhry_1.o: demo/dhry_1.c
	$(CC) $(DHRYSTONE_FLAGS) -o $@ $<

$(OUT)/dhry_2.o: demo/dhry_2.c
	$(CC) $(DHRYSTONE_FLAGS) -o $@ $<

$(OUT)/mandelbrot.o: demo/mandelbrot.c
	$(CC) $(CCFLAGS) $(DEMO_FLAGS) -o $@ $<

$(OUT)/raytrace.o: demo/raytrace.cpp
	$(CXX) $(CXXFLAGS) $(DEMO_FLAGS) -o $@ $<

$(OUT)/retro.o: demo/retro.cpp
	$(CXX) $(CXXFLAGS) $(DEMO_FLAGS) -o $@ $<

$(OUT)/stars.o: demo/stars.cpp
	$(CXX) $(CXXFLAGS) $(DEMO_FLAGS) -o $@ $<

$(OUT)/mrisc32_logo.o: $(OUT)/mrisc32_logo.c
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/mrisc32_logo.c: demo/mrisc32-logo-512x97.png $(HOST_OUT)/png2mci
	$(HOST_OUT)/png2mci --lzg --pal4 demo/mrisc32-logo-512x97.png $(OUT)/mrisc32_logo.mci
	tools/raw2c.py $(OUT)/mrisc32_logo.mci mrisc32_logo > $@


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
    $(OUT)/crc32c.o \
    $(OUT)/doh.o \
    $(OUT)/fast_math.o \
    $(OUT)/framebuffer.o \
    $(OUT)/glyph_renderer.o \
    $(OUT)/keyboard.o \
    $(OUT)/keyboard_layout_en_us.o \
    $(OUT)/leds.o \
    $(OUT)/lzg_mc1.o \
    $(OUT)/mc1-font-8x8.o \
    $(OUT)/mci_decode.o \
    $(OUT)/memory.o \
    $(OUT)/time.o \
    $(OUT)/vconsole.o \
    $(OUT)/vcp.o

$(OUT)/libmc1.a: $(LIBMC1_OBJS)
	$(AR) $(ARFLAGS) $@ $(LIBMC1_OBJS)

$(OUT)/crt0.o: mc1/crt0.s mc1/memory.inc mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ mc1/crt0.s

$(OUT)/crc32c.o: mc1/crc32c.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/doh.o: mc1/doh.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OUT)/fast_math.o: mc1/fast_math.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/framebuffer.o: mc1/framebuffer.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/glyph_renderer.o: mc1/glyph_renderer.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OUT)/keyboard.o: mc1/keyboard.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OUT)/keyboard_layout_en_us.o: mc1/keyboard_layout_en_us.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OUT)/leds.o: mc1/leds.s mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ mc1/leds.s

$(OUT)/lzg_mc1.o: mc1/lzg_mc1.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/mc1-font-8x8.o: $(OUT)/mc1-font-8x8.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/mc1-font-8x8.s: mc1/mc1-font-8x8.raw
	# Trim out ASCII code points 32..127 (the printable ASCII chars).
	cat mc1/mc1-font-8x8.raw | \
	    tail -c +257  | \
	    head -c -1024 \
	    > $(OUT)/mc1-font-8x8-part.raw
	tools/raw2asm.py --rev $(OUT)/mc1-font-8x8-part.raw mc1_font_8x8 > $@

$(OUT)/mci_decode.o: mc1/mci_decode.c
	$(CC) $(CCFLAGS) -o $@ $<

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

