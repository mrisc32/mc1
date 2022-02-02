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
SELFTESTINC = $(SELFTESTDIR)/src

# TODO(m): Remove -Wno-array-bounds once the GCC 12 bug has been fixed upstream. This is a
# temporary workaround to make the MMIO() macro work (i.e. access a constant address).
# See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101379
CFLAGS_COMMON = -c -I $(LIBMC1INC) -Os -ffast-math \
                -Wall -Wextra -Wshadow -Wno-array-bounds -pedantic -Werror \
                -MMD -MP

CC       = mrisc32-elf-gcc
CCFLAGS  = $(CFLAGS_COMMON) -std=c11
CXX      = mrisc32-elf-g++
CXXFLAGS = $(CFLAGS_COMMON) -std=c++17 -Wold-style-cast -fno-exceptions
AS       = mrisc32-elf-gcc
ASFLAGS  = -c -I $(LIBMC1INC)
LD       = mrisc32-elf-gcc
LDFLAGS  = -L$(OUT) -L$(SELFTESTOUT) -T link.ld -mno-crt0 -mno-ctor-dtor
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
# MC1 tools
#-----------------------------------------------------------------------------

PNG2MCI = $(SDK_ROOT)/tools/png2mci
RAW2C   = $(SDK_ROOT)/tools/raw2c.py

$(PNG2MCI):
	@echo "=============================================================================="
	@echo " Please build $(PNG2MCI) (see $(SDK_ROOT)/tools/README.md)"
	@echo "=============================================================================="
	@false


#-----------------------------------------------------------------------------
# ROM image
#-----------------------------------------------------------------------------

# ROM configuration
ENABLE_SPLASH = yes
ENABLE_CONSOLE = no
ENABLE_SELFTEST = no

ROM_OBJS = \
    $(OUT)/crt0.o \
    $(OUT)/elf32.o \
    $(OUT)/main.o

ROM_FLAGS =

ifeq ($(ENABLE_CONSOLE),yes)
  ROM_FLAGS += -DENABLE_CONSOLE
  ifeq ($(ENABLE_SELFTEST),yes)
    ROM_FLAGS += -DENABLE_SELFTEST -I $(SELFTESTINC)
  endif
endif
ifeq ($(ENABLE_SPLASH),yes)
  ROM_FLAGS += -DENABLE_SPLASH
  ROM_OBJS += $(OUT)/boot-splash.o
endif

$(OUT)/crt0.o: crt0.s $(LIBMC1INC)/mc1/memory.inc $(LIBMC1INC)/mc1/mmio.inc
	$(AS) $(ASFLAGS) $(ROM_FLAGS) -o $@ crt0.s

$(OUT)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) $(ROM_FLAGS) -o $@ $<

$(OUT)/elf32.o: elf32.cpp
	$(CXX) $(CXXFLAGS) $(ROM_FLAGS) -o $@ $<

$(OUT)/boot-splash.o: media/boot-splash.png
	$(PNG2MCI) --lzg --pal4 $< $(OUT)/boot-splash.mci
	$(RAW2C) $(OUT)/boot-splash.mci boot_splash_mci > $(OUT)/boot-splash.c
	$(CC) $(CCFLAGS) -o $@ $(OUT)/boot-splash.c

$(OUT)/rom.elf: $(ROM_OBJS) $(OUT)/libmc1.a $(OUT)/libselftest.a link.ld
	$(LD) $(LDFLAGS) -o $@ $(ROM_OBJS) -lmc1 -lselftest -lm

$(OUT)/rom.raw: $(OUT)/rom.elf
	$(OBJCOPY) -O binary $< $@

$(OUT)/rom.vhd: $(OUT)/rom.raw rom.vhd.in
	tools/raw2vhd.py $(OUT)/rom.raw rom.vhd.in > $@


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
-include $(ROM_OBJS:.o=.d)

