# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

BUILDDIR = out

AS      = mrisc32-elf-as
ASFLAGS = -I .
LD      = mrisc32-elf-ld
LDFLAGS = -L$(BUILDDIR)
AR      = mrisc32-elf-ar
ARFLAGS = rcs
OBJCOPY = mrisc32-elf-objcopy

.PHONY: clean all

all: ../rtl/rom.vhd

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/*.a $(BUILDDIR)/*.elf $(BUILDDIR)/*.raw

APPOBJS = $(BUILDDIR)/main.o

SYSOBJS = $(BUILDDIR)/crt0.o \
          $(BUILDDIR)/leds.o \
          $(BUILDDIR)/time.o

../rtl/rom.vhd: $(BUILDDIR)/rom.raw rom.vhd.in
	./raw2vhd.py $(BUILDDIR)/rom.raw rom.vhd.in > $@

$(BUILDDIR)/rom.raw: $(BUILDDIR)/rom.elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/rom.elf: $(APPOBJS) $(BUILDDIR)/libsys.a
	$(LD) $(LDFLAGS) -o $@ $(APPOBJS) -lsys

$(BUILDDIR)/libsys.a: $(SYSOBJS)
	$(AR) $(ARFLAGS) $@ $(SYSOBJS)

$(BUILDDIR)/crt0.o: system/crt0.s system/memory.inc system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/crt0.s

$(BUILDDIR)/leds.o: system/leds.s system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/leds.s

$(BUILDDIR)/time.o: system/time.s system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/time.s

$(BUILDDIR)/main.o: app/main.s system/memory.inc
	$(AS) $(ASFLAGS) -o $@ app/main.s


