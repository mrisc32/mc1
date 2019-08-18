#!/usr/bin/env python3

import argparse
import struct


def convert(infile, outfile):
    # Read the input file.
    with open(infile, "rb") as f:
        data = f.read()
        colors = []
        for k in range(0, len(data), 3):
            # Convert to ABGR32 (RGBA8888)
            colors.append(0xff000000 | (data[k + 2] << 16) | (data[k + 1] << 8) | data[k])

    print(f"Number of colors: {len(colors)}")

    # Write the output file.
    with open(outfile, "wb") as f:
        for color in colors:
            f.write(struct.pack("<I", color))


def main():
    # Parse command line arguments.
    parser = argparse.ArgumentParser(
            description='Convert an RGB888 palette to an RGBA8888 palette file')
    parser.add_argument('infile', metavar='INFILE', help='the 24-bit palette file (raw)')
    parser.add_argument('outfile', metavar='OUTFILE', help='the 32-bit palette file (raw)')
    args = parser.parse_args()

    # Assemble the file.
    convert(args.infile, args.outfile)


if __name__ == '__main__':
    main()
