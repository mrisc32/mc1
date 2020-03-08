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

APP1OBJS = $(OUT)/app1.o \
           $(OUT)/funky.o \
           $(OUT)/mandelbrot.o \
           $(OUT)/raytrace.o

APP2OBJS = $(OUT)/app2.o

# Select which app to build into the ROM here:
APPOBJS = $(APP1OBJS)

$(OUT)/app1.s: app1/app1.c
	$(CC) $(CCFLAGS) -o $@ app1/app1.c

$(OUT)/app1.o: $(OUT)/app1.s
	$(AS) $(ASFLAGS) -o $@ $(OUT)/app1.s

$(OUT)/funky.o: app1/funky.s
	$(AS) $(ASFLAGS) -o $@ app1/funky.s

$(OUT)/mandelbrot.o: app1/mandelbrot.s
	$(AS) $(ASFLAGS) -o $@ app1/mandelbrot.s

$(OUT)/raytrace.s: app1/raytrace.c
	$(CC) $(CCFLAGS) -o $@ app1/raytrace.c

$(OUT)/raytrace.o: $(OUT)/raytrace.s
	$(AS) $(ASFLAGS) -o $@ $(OUT)/raytrace.s


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
          $(OUT)/framebuffer.o \
          $(OUT)/leds.o \
          $(OUT)/mc1-font-8x8.o \
          $(OUT)/mem_copy.o \
          $(OUT)/mem_fill.o \
          $(OUT)/memory.o \
          $(OUT)/time.o \
          $(OUT)/vconsole.o

$(OUT)/libsys.a: $(SYSOBJS)
	$(AR) $(ARFLAGS) $@ $(SYSOBJS)

$(OUT)/crt0.o: mc1/crt0.s mc1/memory.inc mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ mc1/crt0.s

$(OUT)/framebuffer.s: mc1/framebuffer.c mc1/memory.h
	$(CC) $(CCFLAGS) -o $@ mc1/framebuffer.c

$(OUT)/framebuffer.o: $(OUT)/framebuffer.s
	$(AS) $(ASFLAGS) -o $@ $(OUT)/framebuffer.s

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
	./raw2asm.py --rev $(OUT)/mc1-font-8x8-part.raw mc1_font_8x8 > $@

$(OUT)/mem_copy.o: mc1/mem_copy.s
	$(AS) $(ASFLAGS) -o $@ mc1/mem_copy.s

$(OUT)/mem_fill.o: mc1/mem_fill.s
	$(AS) $(ASFLAGS) -o $@ mc1/mem_fill.s

$(OUT)/memory.s: mc1/memory.c
	$(CC) $(CCFLAGS) -o $@ mc1/memory.c

$(OUT)/memory.o: $(OUT)/memory.s
	$(AS) $(ASFLAGS) -o $@ $(OUT)/memory.s

$(OUT)/time.o: mc1/time.s mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ mc1/time.s

$(OUT)/vconsole.o: mc1/vconsole.s mc1/mmio.inc mc1/memory.inc
	$(AS) $(ASFLAGS) -o $@ mc1/vconsole.s

