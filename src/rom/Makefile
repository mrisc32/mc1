# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

BUILDDIR = out

CC      = mrisc32-elf-gcc
CCFLAGS = -S -Os -I . -ffreestanding -Wall -Wextra -Werror
AS      = mrisc32-elf-as
ASFLAGS = -I .
LD      = mrisc32-elf-ld
LDFLAGS = -L$(BUILDDIR) --script link.ld
AR      = mrisc32-elf-ar
ARFLAGS = rcs
OBJCOPY = mrisc32-elf-objcopy

.PHONY: clean all

all: ../rtl/rom.vhd

clean:
	rm -f $(BUILDDIR)/*.s \
	      $(BUILDDIR)/*.o \
	      $(BUILDDIR)/*.a \
	      $(BUILDDIR)/*.elf \
	      $(BUILDDIR)/*.raw

APP1OBJS = $(BUILDDIR)/app1.o

APP2OBJS = $(BUILDDIR)/app2.o

# Select which app to build into the ROM here:
APPOBJS = $(APP1OBJS)

SYSOBJS = $(BUILDDIR)/crt0.o \
          $(BUILDDIR)/leds.o \
          $(BUILDDIR)/time.o \
          $(BUILDDIR)/memory.o

../rtl/rom.vhd: $(BUILDDIR)/rom.raw rom.vhd.in
	./raw2vhd.py $(BUILDDIR)/rom.raw rom.vhd.in > $@

$(BUILDDIR)/rom.raw: $(BUILDDIR)/rom.elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/rom.elf: $(APPOBJS) $(BUILDDIR)/libsys.a link.ld
	$(LD) $(LDFLAGS) -o $@ $(APPOBJS) -lsys

$(BUILDDIR)/libsys.a: $(SYSOBJS)
	$(AR) $(ARFLAGS) $@ $(SYSOBJS)

$(BUILDDIR)/crt0.o: system/crt0.s system/memory.inc system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/crt0.s

$(BUILDDIR)/leds.o: system/leds.s system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/leds.s

$(BUILDDIR)/time.o: system/time.s system/mmio.inc
	$(AS) $(ASFLAGS) -o $@ system/time.s

$(BUILDDIR)/memory.s: system/memory.c
	$(CC) $(CCFLAGS) -o $@ system/memory.c

$(BUILDDIR)/memory.o: $(BUILDDIR)/memory.s
	$(AS) $(ASFLAGS) -o $@ $(BUILDDIR)/memory.s

$(BUILDDIR)/app1.o: app1/app1.s system/memory.inc
	$(AS) $(ASFLAGS) -o $@ app1/app1.s

$(BUILDDIR)/app2.s: app2/app2.c
	$(CC) $(CCFLAGS) -o $@ app2/app2.c

$(BUILDDIR)/app2.o: $(BUILDDIR)/app2.s
	$(AS) $(ASFLAGS) -o $@ $(BUILDDIR)/app2.s

