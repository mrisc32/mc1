# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-
#--------------------------------------------------------------------------------------------------
# Copyright (c) 2021 Marcus Geelnard
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

AS      = mrisc32-elf-as
OBJCOPY = mrisc32-elf-objcopy

OUT     = ./out

.PHONY: clean all

all: $(OUT)/boot.img

clean:
	rm -f $(OUT)/*

SRC = boot-pictures.s

PICS = files/butterfly-1200x675-2bpp.raw \
       files/currant-1200x675-2bpp.raw

$(OUT)/boot.o: $(SRC)
	$(AS) -o $@ $<

$(OUT)/boot.img: $(OUT)/boot.o $(PICS)
	$(OBJCOPY) -O binary $< $(OUT)/boot.raw
	./mkmc1bootimage.py $(OUT)/boot.raw $@ $(PICS)

