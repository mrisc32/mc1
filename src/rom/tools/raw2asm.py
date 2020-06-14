#!/usr/bin/env python3
# -*- mode: python; tab-width: 4; indent-tabs-mode: nil; -*-

import argparse
import struct
from pathlib import Path


def reverse_bits(x):
    return ((x >> 7) & 1) | ((x >> 5) & 2) | ((x >> 3) & 4) | ((x >> 1) & 8) | ((x << 1) & 16) | ((x << 3) & 32) | ((x << 5) & 64) | ((x << 7) & 128)


def convert(raw_filename, symbol, rev):
    # Read the raw file.
    with open(raw_filename, 'rb') as f:
        raw_data = f.read()

    # Start of the source file.
    asm_source = f'\t; This file is generated from {Path(raw_filename).parts[-1]}\n\n'
    asm_source += '\t.section\t.rodata\n'
    asm_source += f'\t.p2align\t2\n'
    asm_source += f'\t.globl\t{symbol}\n'
    asm_source += f'{symbol}:'

    # Generate the data statements.
    raw_data_8bit = struct.unpack('B' * len(raw_data), raw_data)
    col = 99
    for x in raw_data_8bit:
        if rev:
            x = reverse_bits(x)
        if col >= 8:
            asm_source = asm_source + '\n\t.byte\t'
            col = 0
        else:
            asm_source = asm_source + ', '
        asm_source = asm_source + f'0x{x:02x}'
        col = col + 1

    print(asm_source)


def main():
    # Parse command line arguments.
    parser = argparse.ArgumentParser(
            description='Convert a raw file to an assembler source file')
    parser.add_argument('raw', metavar='RAW_FILE', help='the raw file to convert')
    parser.add_argument('symbol', metavar='SYMBOL', help='the data symbol name')
    parser.add_argument('--rev', action='store_true', help='reverse the bits of each byte')
    args = parser.parse_args()

    # Convert the file.
    convert(args.raw, args.symbol, args.rev)


if __name__ == "__main__":
    main()
