# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

AS      = mrisc32-elf-as
ASFLAGS = -I src
LD      = mrisc32-elf-ld
AR      = mrisc32-elf-ar
ARFLAGS = rcs
OBJCOPY = mrisc32-elf-objcopy

BUILDDIR = out

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
	$(LD) -L$(BUILDDIR) -o $@ $(APPOBJS) -lsys

$(BUILDDIR)/libsys.a: $(SYSOBJS)
	$(AR) $(ARFLAGS) $@ $(SYSOBJS)

$(BUILDDIR)/crt0.o: src/system/crt0.s src/system/memory.inc src/system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ src/system/crt0.s

$(BUILDDIR)/leds.o: src/system/leds.s src/system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ src/system/leds.s

$(BUILDDIR)/time.o: src/system/time.s src/system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ src/system/time.s

$(BUILDDIR)/main.o: src/app/main.s src/system/memory.inc
	$(AS) $(ASFLAGS) -o $@ src/app/main.s


