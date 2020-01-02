# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

OUT = out
SELFTESTOUT = selftest/out

CC      = mrisc32-elf-gcc
CCFLAGS = -S -Os -I . -I libc -ffreestanding -Wall -Wextra -Werror
AS      = mrisc32-elf-as
ASFLAGS = -I .
LD      = mrisc32-elf-ld
LDFLAGS = -L$(OUT) -L$(SELFTESTOUT) --script link.ld
AR      = mrisc32-elf-ar
ARFLAGS = rcs
OBJCOPY = mrisc32-elf-objcopy
CP      = cp -a

.PHONY: clean all selftest

all: $(OUT)/rom.vhd

clean:
	rm -f $(OUT)/*.s \
	      $(OUT)/*.o \
	      $(OUT)/*.a \
	      $(OUT)/*.elf \
	      $(OUT)/*.raw \
	      $(OUT)/*.vhd
	$(MAKE) -C selftest clean


#-----------------------------------------------------------------------------
# Applications
#-----------------------------------------------------------------------------

APP1OBJS = $(OUT)/app1.o

APP2OBJS = $(OUT)/app2.o

# Select which app to build into the ROM here:
APPOBJS = $(APP1OBJS)

$(OUT)/app1.o: app1/app1.s system/memory.inc
	$(AS) $(ASFLAGS) -o $@ app1/app1.s

$(OUT)/app2.s: app2/app2.c
	$(CC) $(CCFLAGS) -o $@ app2/app2.c

$(OUT)/app2.o: $(OUT)/app2.s
	$(AS) $(ASFLAGS) -o $@ $(OUT)/app2.s


#-----------------------------------------------------------------------------
# ROM image
#-----------------------------------------------------------------------------

$(OUT)/rom.vhd: $(OUT)/rom.raw rom.vhd.in
	./raw2vhd.py $(OUT)/rom.raw rom.vhd.in > $@

$(OUT)/rom.raw: $(OUT)/rom.elf
	$(OBJCOPY) -O binary $< $@

$(OUT)/rom.elf: $(APPOBJS) $(OUT)/libsys.a $(OUT)/libselftest.a link.ld
	$(LD) $(LDFLAGS) -o $@ $(APPOBJS) -lsys -lselftest


#-----------------------------------------------------------------------------
# libselftest.a - Selftest library
#-----------------------------------------------------------------------------

$(OUT)/libselftest.a: selftest
	@$(CP) $(SELFTESTOUT)/libselftest.a $(OUT)/libselftest.a

selftest:
	$(MAKE) -C selftest


#-----------------------------------------------------------------------------
# libsys.a - System library
#-----------------------------------------------------------------------------

SYSOBJS = $(OUT)/crt0.o \
          $(OUT)/mc1-font-8x8.o \
          $(OUT)/leds.o \
          $(OUT)/mem_copy.o \
          $(OUT)/mem_fill.o \
          $(OUT)/memory.o \
          $(OUT)/time.o \
          $(OUT)/vconsole.o

$(OUT)/libsys.a: $(SYSOBJS)
	$(AR) $(ARFLAGS) $@ $(SYSOBJS)

$(OUT)/crt0.o: system/crt0.s system/memory.inc system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/crt0.s

$(OUT)/leds.o: system/leds.s system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/leds.s

$(OUT)/mc1-font-8x8.o: $(OUT)/mc1-font-8x8.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/mc1-font-8x8.s: system/mc1-font-8x8.raw
	# Trim out ASCII code points 32..127 (the printable ASCII chars).
	cat system/mc1-font-8x8.raw | \
	    tail -c +257  | \
	    head -c -1024 \
	    > $(OUT)/mc1-font-8x8-part.raw
	./raw2asm.py --rev $(OUT)/mc1-font-8x8-part.raw mc1_font_8x8 > $@

$(OUT)/mem_copy.o: system/mem_copy.s
	$(AS) $(ASFLAGS) -o $@ system/mem_copy.s

$(OUT)/mem_fill.o: system/mem_fill.s
	$(AS) $(ASFLAGS) -o $@ system/mem_fill.s

$(OUT)/memory.s: system/memory.c
	$(CC) $(CCFLAGS) -o $@ system/memory.c

$(OUT)/memory.o: $(OUT)/memory.s
	$(AS) $(ASFLAGS) -o $@ $(OUT)/memory.s

$(OUT)/time.o: system/time.s system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/time.s

$(OUT)/vconsole.o: system/vconsole.s system/mmio.inc system/memory.inc
	$(AS) $(ASFLAGS) -o $@ system/vconsole.s

