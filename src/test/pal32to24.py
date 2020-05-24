#!/usr/bin/env python3

import argparse
import struct


def convert(infile, outfile):
    # Read the input file.
    with open(infile, "rb") as f:
        data = f.read()
        colors = []
        for k in range(0, len(data), 4):
            colors.append(data[k])
            colors.append(data[k + 1])
            colors.append(data[k + 2])

    print(f"Number of colors: {len(colors)/3}")

    # Write the output file.
    with open(outfile, "wb") as f:
        f.write(bytes(colors))


def main():
    # Parse command line arguments.
    parser = argparse.ArgumentParser(
            description='Convert an RGBA8888 palette to an RGB888 palette file')
    parser.add_argument('infile', metavar='INFILE', help='the 32-bit palette file (raw)')
    parser.add_argument('outfile', metavar='OUTFILE', help='the 24-bit palette file (raw)')
    args = parser.parse_args()

    # Assemble the file.
    convert(args.infile, args.outfile)


if __name__ == '__main__':
    main()
