#!/usr/bin/env python3
# -*- mode: python; tab-width: 4; indent-tabs-mode: nil; -*-
# --------------------------------------------------------------------------------------------------
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
# --------------------------------------------------------------------------------------------------

import argparse
import crc32c
import struct

_MAX_CODE_SIZE = 512 - 8


def convert(raw, img):
    # Read the raw data.
    with open(raw, "rb") as f:
        data = f.read()
    code_size = len(data)

    # Pad the data to _MAX_CODE_SIZE bytes.
    if code_size > _MAX_CODE_SIZE:
        print(f"Code size is too large: {len(data)} (max {_MAX_CODE_SIZE})")
    pad = _MAX_CODE_SIZE - code_size
    if pad > 0:
        data += bytearray(pad)

    # Prepend the header.
    magic = 0x4231434D
    crc = crc32c.crc32c(data)
    data = struct.pack("<LL", magic, crc) + data

    print("--------------------------------------------------------------------")
    print(f"Code size: {code_size} bytes (padded to {len(data)} bytes)")
    print(f"Magic ID:  {magic:#08x}")
    print(f"CRC32C:    {crc:#08x}")
    print("--------------------------------------------------------------------")

    # Write the boot image.
    with open(img, "wb") as f:
        f.write(data)

    print(f"\nBoot image written to {img}")


def main():
    # Parse command line arguments.
    parser = argparse.ArgumentParser(
        description="Convert a raw file to a boot block image"
    )
    parser.add_argument("raw", metavar="RAW_FILE", help="the raw file to convert")
    parser.add_argument("img", metavar="IMAGE", help="the boot image")
    args = parser.parse_args()

    # Convert the file.
    convert(args.raw, args.img)


if __name__ == "__main__":
    main()
