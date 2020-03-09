#!/usr/bin/env python3
# -*- mode: python; tab-width: 4; indent-tabs-mode: nil; -*-

import argparse
import math
import struct

_RAW_BASE_ADDRESS = 512


def closest_pot(x):
    res = 2
    while x > res:
        res = res * 2
    return res


def convert(raw_filename, template_filename):
    # Read the raw rom file and pad start and end with zeros to account for start address and
    # a power-of-two size.
    with open(raw_filename, 'rb') as f:
        raw_data = f.read()
    raw_data = bytearray(_RAW_BASE_ADDRESS) + raw_data
    rom_size = int(closest_pot(len(raw_data)))
    raw_data = raw_data + bytearray(rom_size - len(raw_data))

    # Derive dynamic data.
    ADDR_BITS = str(int(math.log(rom_size, 2) - 2))
    DATA = ''
    raw_data_32bit = struct.unpack('<' + ('I' * (rom_size // 4)), raw_data)
    for x in range(0, len(raw_data_32bit)):
        tail = '' if x == (len(raw_data_32bit) - 1) else ',\n'
        word = raw_data_32bit[x]
        DATA = DATA + (f'    x"{word:08x}"{tail}')

    # Read the VHDL template.
    with open(template_filename, 'r', encoding='utf8') as f:
        template = f.readlines()

    # Generate the output.
    for l in template:
        l = l.rstrip()
        l = l.replace("${ADDR_BITS}", ADDR_BITS)
        l = l.replace("${DATA}", DATA)
        print(l)


def main():
    # Parse command line arguments.
    parser = argparse.ArgumentParser(
            description='Convert a raw file to a VHDL ROM file')
    parser.add_argument('raw', metavar='RAW_FILE', help='the raw file to convert')
    parser.add_argument('template', metavar='TEMPLATE_FILE', help='the VHDL template file')
    args = parser.parse_args()

    # Convert the file.
    convert(args.raw, args.template)


if __name__ == "__main__":
    main()
