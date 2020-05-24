# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

OUT = out
SELFTESTOUT = selftest/out

CC      = mrisc32-elf-gcc
CCFLAGS = -c -Os -I . -ffast-math -Wall -Wextra -Werror
AS      = mrisc32-elf-gcc
ASFLAGS = -c -I .
LD      = mrisc32-elf-gcc
LDFLAGS = -L$(OUT) -L$(SELFTESTOUT) -T link.ld -mno-crt0
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
# Demo application
#-----------------------------------------------------------------------------

DEMOOBJS = $(OUT)/demo.o \
           $(OUT)/palette_util.o \
           $(OUT)/funky.o \
           $(OUT)/mandelbrot.o \
           $(OUT)/raytrace.o

$(OUT)/demo.o: demo/demo.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/palette_util.o: demo/palette_util.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/funky.o: demo/funky.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/mandelbrot.o: demo/mandelbrot.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/raytrace.o: demo/raytrace.c
	$(CC) $(CCFLAGS) -o $@ $<


#-----------------------------------------------------------------------------
# ROM image
#-----------------------------------------------------------------------------

$(OUT)/rom.vhd: $(OUT)/rom.raw rom.vhd.in
	./raw2vhd.py $(OUT)/rom.raw rom.vhd.in > $@

$(OUT)/rom.raw: $(OUT)/rom.elf
	$(OBJCOPY) -O binary $< $@

$(OUT)/rom.elf: $(DEMOOBJS) $(OUT)/libmc1.a $(OUT)/libselftest.a link.ld
	$(LD) $(LDFLAGS) -o $@ $(DEMOOBJS) -lmc1 -lselftest -lm


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

MC1OBJS = $(OUT)/crt0.o \
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

$(OUT)/libmc1.a: $(MC1OBJS)
	$(AR) $(ARFLAGS) $@ $(MC1OBJS)

$(OUT)/crt0.o: mc1/crt0.s mc1/memory.inc mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ mc1/crt0.s

$(OUT)/fast_math.o: mc1/fast_math.c mc1/fast_math.h
	$(CC) $(CCFLAGS) -o $@ mc1/fast_math.c

$(OUT)/framebuffer.o: mc1/framebuffer.c mc1/memory.h mc1/vcp.h
	$(CC) $(CCFLAGS) -o $@ mc1/framebuffer.c

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

$(OUT)/memory.o: mc1/memory.c
	$(CC) $(CCFLAGS) -o $@ mc1/memory.c

$(OUT)/time.o: mc1/time.s mc1/mmio.inc
	$(AS) $(ASFLAGS) -o $@ mc1/time.s

$(OUT)/vconsole.o: mc1/vconsole.s mc1/mmio.inc mc1/memory.inc
	$(AS) $(ASFLAGS) -o $@ mc1/vconsole.s

$(OUT)/vcp.o: mc1/vcp.c
	$(CC) $(CCFLAGS) -o $@ mc1/vcp.c

