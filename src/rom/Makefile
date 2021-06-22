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
SDK_ROOT    = ../mc1-sdk
LIBMC1DIR   = $(SDK_ROOT)/libmc1
LIBMC1INC   = $(LIBMC1DIR)/include
LIBMC1OUT   = $(LIBMC1DIR)/out
SELFTESTDIR = ../selftest
SELFTESTOUT = $(SELFTESTDIR)/out

CFLAGS_COMMON = -c -I $(LIBMC1INC) -Os -ffast-math -Wall -Wextra -Wshadow -pedantic -Werror -MMD -MP

CC       = mrisc32-elf-gcc
CCFLAGS  = $(CFLAGS_COMMON) -std=c11
CXX      = mrisc32-elf-g++
CXXFLAGS = $(CFLAGS_COMMON) -std=c++17 -Wold-style-cast -fno-exceptions
AS       = mrisc32-elf-gcc
ASFLAGS  = -c -I $(LIBMC1INC)
LD       = mrisc32-elf-gcc
LDFLAGS  = -L$(OUT) -L$(SELFTESTOUT) -T link.ld -mno-crt0
AR       = mrisc32-elf-ar
ARFLAGS  = rcs
OBJCOPY  = mrisc32-elf-objcopy
CP       = cp -a

DHRYSTONE_FLAGS = -S -w -fno-inline -O3

.PHONY: clean all libmc1 selftest

all: $(OUT)/rom.vhd

clean:
	rm -f $(OUT)/*.a \
	      $(OUT)/*.c \
	      $(OUT)/*.d \
	      $(OUT)/*.s \
	      $(OUT)/*.o \
	      $(OUT)/*.elf \
	      $(OUT)/*.mci \
	      $(OUT)/*.raw \
	      $(OUT)/*.vhd
	$(MAKE) -C $(LIBMC1DIR) clean
	$(MAKE) -C $(SELFTESTDIR) clean


#-----------------------------------------------------------------------------
# Demo application
#-----------------------------------------------------------------------------

# Demo configuration.
ENABLE_DEMO = yes
DEMO_INTERACTIVE_MODE = yes
DEMO_ENABLE_SELFTEST = no
DEMO_ENABLE_DHRYSTONE = no

DEMO_OBJS =
ifeq ($(ENABLE_DEMO),yes)
  DEMO_OBJS += \
      $(OUT)/console.o \
      $(OUT)/demo.o \
      $(OUT)/mandelbrot.o \
      $(OUT)/mrisc32_logo.o \
      $(OUT)/raytrace.o \
      $(OUT)/retro.o \
      $(OUT)/stars.o

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
endif

$(OUT)/console.o: demo/console.cpp
	$(CXX) $(CXXFLAGS) $(DEMO_FLAGS) -o $@ $<

$(OUT)/demo.o: demo/demo.c
	$(CC) $(CCFLAGS) $(DEMO_FLAGS) -o $@ $<

$(OUT)/dhry_1.s: demo/dhry_1.c
	$(CC) $(DHRYSTONE_FLAGS) -o $@ $<

$(OUT)/dhry_1.o: $(OUT)/dhry_1.s
	$(CC) -c -o $@ $<

$(OUT)/dhry_2.s: demo/dhry_2.c
	$(CC) $(DHRYSTONE_FLAGS) -o $@ $<

$(OUT)/dhry_2.o: $(OUT)/dhry_2.s
	$(CC) -c -o $@ $<

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

$(OUT)/mrisc32_logo.c: demo/mrisc32-logo-512x97.png
	$(SDK_ROOT)/tools/png2mci --lzg --pal4 demo/mrisc32-logo-512x97.png $(OUT)/mrisc32_logo.mci
	$(SDK_ROOT)/tools/raw2c.py $(OUT)/mrisc32_logo.mci mrisc32_logo > $@


#-----------------------------------------------------------------------------
# ROM image
#-----------------------------------------------------------------------------

ROM_OBJS = \
    $(OUT)/crt0.o \
    $(OUT)/doh.o

ROM_FLAGS =
ifeq ($(ENABLE_DEMO),yes)
  ROM_FLAGS += -DENABLE_DEMO -Wa,--defsym,ENABLE_DEMO=1
endif

$(OUT)/crt0.o: crt0.s $(LIBMC1INC)/mc1/memory.inc $(LIBMC1INC)/mc1/mmio.inc
	$(AS) $(ASFLAGS) $(ROM_FLAGS) -o $@ crt0.s

$(OUT)/doh.o: doh.cpp
	$(CXX) $(CXXFLAGS) $(ROM_FLAGS) -o $@ $<

$(OUT)/rom.vhd: $(OUT)/rom.raw rom.vhd.in
	tools/raw2vhd.py $(OUT)/rom.raw rom.vhd.in > $@

$(OUT)/rom.raw: $(OUT)/rom.elf
	$(OBJCOPY) -O binary $< $@

$(OUT)/rom.elf: $(DEMO_OBJS) $(ROM_OBJS) $(OUT)/libmc1.a $(OUT)/libselftest.a link.ld
	$(LD) $(LDFLAGS) -o $@ $(DEMO_OBJS) $(ROM_OBJS) -lmc1 -lselftest -lm


#-----------------------------------------------------------------------------
# libmc1.a
#-----------------------------------------------------------------------------

$(OUT)/libmc1.a: libmc1
	@$(CP) $(LIBMC1OUT)/libmc1.a $(OUT)/libmc1.a

libmc1:
	$(MAKE) CFLAGS_OPT='-Os' -C $(LIBMC1DIR)


#-----------------------------------------------------------------------------
# libselftest.a - Selftest library
#-----------------------------------------------------------------------------

$(OUT)/libselftest.a: selftest
	@$(CP) $(SELFTESTOUT)/libselftest.a $(OUT)/libselftest.a

selftest:
	$(MAKE) -C $(SELFTESTDIR)


# Include dependency files (generated when building the object files).
-include $(PNG2MCI_OBJS:.o=.d)
-include $(DEMO_OBJS:.o=.d)

