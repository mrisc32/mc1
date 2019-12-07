# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

AS      = mrisc32-elf-as
ASFLAGS = -I src
LD      = mrisc32-elf-ld
OBJCOPY = mrisc32-elf-objcopy

BUILDDIR = out

.PHONY: clean all

all: ../rtl/rom.vhd

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/*.elf

OBJS = $(BUILDDIR)/crt0.o \
       $(BUILDDIR)/main.o \
       $(BUILDDIR)/libsys.o

../rtl/rom.vhd: $(BUILDDIR)/rom.raw rom.vhd.in
	./raw2vhd.py $(BUILDDIR)/rom.raw rom.vhd.in > $@

$(BUILDDIR)/rom.raw: $(BUILDDIR)/rom.elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/rom.elf: $(BUILDDIR)/crt0.o $(BUILDDIR)/main.o $(BUILDDIR)/libsys.o
	$(LD) -o $@ $(OBJS)

$(BUILDDIR)/crt0.o: src/crt0.s src/config.inc
	$(AS) $(ASFLAGS) -o $@ src/crt0.s

$(BUILDDIR)/main.o: src/main.s src/config.inc
	$(AS) $(ASFLAGS) -o $@ src/main.s

$(BUILDDIR)/libsys.o: src/libsys.s src/config.inc
	$(AS) $(ASFLAGS) -o $@ src/libsys.s

