#!/usr/bin/env python3
# -*- mode: python; tab-width: 4; indent-tabs-mode: nil; -*-

import argparse
import math
import os
import struct
import sys


class _OutputFormat:
    GNU_ASM = 1
    BIN = 2


_PUBLIC_EVAL_FUNCTIONS = {
    "sin": math.sin,
    "cos": math.cos
}


def strip_arg(arg):
    arg = arg.strip()
    if arg[0] == "\"":
        return arg[1:-1]
    return arg


def included_file_path(parent_file, included_file):
    return os.path.join(os.path.dirname(os.path.realpath(parent_file)), included_file)


def incbin(file_name, line_no):
    with open(file_name, "rb") as f:
        data = f.read()
        num_words = int(len(data) / 4)
        # TODO(m): Pad an extra word with zeros for non multiples of four bytes.
        words = struct.unpack("<" + "I" * num_words, data)

    statements = []
    for word in words:
        statements.append({"line": line_no, "cmd": ".word", "args": [f"{word:#10x}"]})

    return statements


def parse_vcp_file(vcp_file):
    statements = []
    line_no = 0
    with open(vcp_file, mode="rt", encoding="utf-8") as f:
        for line in f:
            line_no = line_no + 1

            # Read and strip the line from comments and whitespace.
            comment_start = line.find(";")
            if comment_start >= 0:
                line = line[:comment_start]
            line = line.strip()
            if not line:
                continue

            # Extract command and operands.
            first_space_pos = line.find(" ")
            if first_space_pos >= 0:
                cmd = line[:first_space_pos]
                args_str = line[first_space_pos:].strip()
                args = [strip_arg(arg) for arg in args_str.split(",")]
            else:
                cmd = line
                args = []

            # The command is always case insensitive.
            cmd = cmd.lower()

            # Handle include directives.
            if cmd == ".incbin":
                statements.extend(incbin(included_file_path(vcp_file, args[0]), line_no))
            elif cmd == ".include":
                statements.extend(parse_vcp_file(included_file_path(vcp_file, args[0])))
            else:
                statements.append({"line": line_no, "cmd": cmd, "args": args})

    return statements


def eval_expr(expr, labels, symbols):
    return int(eval(expr, {**labels, **symbols, **_PUBLIC_EVAL_FUNCTIONS}))


def eval_args(args, labels, symbols):
    return [eval_expr(arg, labels, symbols) for arg in args]


def lerp8(a, b, w):
    return int(round(a + (b - a) * w)) & 255


def lerp(first, last, count):
    assert(count >= 1)
    if count == 1:
        return [first]

    result = []
    rgba0 = [(first >> 24) & 255,
             (first >> 16) & 255,
             (first >> 8) & 255,
             first & 255]
    rgba1 = [(last >> 24) & 255,
             (last >> 16) & 255,
             (last >> 8) & 255,
             last & 255]
    for k in range(0, count):
        w = k / (count - 1)
        rgba = [lerp8(rgba0[0], rgba1[0], w),
                lerp8(rgba0[1], rgba1[1], w),
                lerp8(rgba0[2], rgba1[2], w),
                lerp8(rgba0[3], rgba1[3], w)]
        result.append((rgba[0] << 24) | (rgba[1] << 16) | (rgba[2] << 8) | rgba[3])

    return result


def translate_command(cmd, args):
    if cmd == "jmp":
        code = 0x00000000 | (args[0] & 0x00ffffff)
    elif cmd == "jsr":
        code = 0x10000000 | (args[0] & 0x00ffffff)
    elif cmd == "rts":
        code = 0x20000000
    elif cmd == "nop":
        code = 0x30000000
    elif cmd == "waitx":
        code = 0x40000000 | (args[0] & 0x0000ffff)
    elif cmd == "waity":
        code = 0x50000000 | (args[0] & 0x0000ffff)
    elif cmd == "setpal":
        code = 0x60000000 | ((args[0] & 255) << 8) | ((args[1] - 1) & 255)
    elif cmd == "setreg":
        code = 0x80000000 | ((args[0] & 15) << 24) | (args[1] & 0x00ffffff)
    else:
        raise Exception(f"Unrecognized command: {cmd}")

    return code


def translate_code(statements):
    # First, collect all label names (set label address to zero)
    labels = {}
    for statement in statements:
        cmd = statement["cmd"]
        if cmd[-1:] == ":":
            label = cmd[:-1]
            labels[label] = 0

    # We do two passes:
    #  1st pass: Collect label addresses
    #  2nd pass: Generate code
    words = []
    start = None
    for pass_no in [1, 2]:
        first_pass = (pass_no == 1)
        symbols = {}
        rept_start = None
        statement_no = 0
        while statement_no < len(statements):
            statement = statements[statement_no]
            line = statement["line"]
            cmd = statement["cmd"]
            args = statement["args"]
            try:
                if cmd[-1:] == ":":
                    if first_pass:
                        label = cmd[:-1]
                        labels[label] = pc

                elif cmd == ".org":
                    pc = eval_expr(args[0], labels, symbols)
                    if not start:
                        start = pc

                elif cmd == ".set":
                    symbols[args[0]] = eval_expr(args[1], labels, symbols)

                elif cmd == ".word":
                    for arg in args:
                        if not first_pass:
                            words.append(eval_expr(arg, labels, symbols))
                        pc = pc + 1

                elif cmd == ".lerp":
                    first = eval_expr(args[0], labels, symbols)
                    last = eval_expr(args[1], labels, symbols)
                    count = eval_expr(args[2], labels, symbols)
                    lerp_words = lerp(first, last, count)
                    if not first_pass:
                        words.extend(lerp_words)
                    pc = pc + len(lerp_words)

                elif cmd == ".rept":
                    if rept_start:
                        raise Exception("Nested .rept statements are not allowed")
                    rept_start = statement_no
                    rept_count = eval_expr(args[0], labels, symbols)
                    if rept_count < 1:
                        raise Exception(f"Invalid .rept count: {rept_count}")

                elif cmd == ".endr":
                    if not rept_start:
                        raise Exception(".endr without .rept is not allowed")
                    rept_count = rept_count - 1
                    if rept_count > 0:
                        statement_no = rept_start
                    else:
                        rept_start = None

                elif cmd[0] == ".":
                    raise Exception(f"Unrecognized directive: {cmd}")

                else:
                    if not first_pass:
                        words.append(translate_command(cmd, eval_args(args, labels, symbols)))
                    pc = pc + 1

                statement_no = statement_no + 1

            except:
                print(f"line {line}: Parse error:", sys.exc_info())
                sys.exit(1)

    return { "words": words, "start": start }


def write_asm(output_file, code, vcp_file):
    with open(output_file, mode="wt", encoding="utf-8") as f:
        # TODO(m): Add a more flexible way to export symbols.
        f.write(f"; Source file: {vcp_file}\n")
        f.write(f"; Assembled by vcpas\n\n")
        f.write(f"    .data\n\n")
        f.write(f"    .global vcp_program\n")
        f.write(f"    .global vcp_program_start\n")
        f.write(f"    .global vcp_program_words\n\n")
        f.write(f"vcp_program_start = {code['start']:#08x}\n")
        f.write(f"vcp_program_words = {len(code['words'])}\n\n")
        f.write(f"vcp_program:\n")
        for word in code["words"]:
            f.write(f"    .word   {word:#010x}\n")


def write_bin(output_file, code):
    with open(output_file, mode="wb") as f:
        for word in code["words"]:
            f.write(struct.pack("<I", word))


def get_format(format, output_file):
    # Auto-detect the format?
    if format == "auto":
        _, file_ext = os.path.splitext(output_file)
        if file_ext.lower() in [".s", ".inc"]:
            return _OutputFormat.GNU_ASM
        else:
            return _OutputFormat.BIN

    # Convert the format string to an internal enum.
    if format == "asm":
        return _OutputFormat.GNU_ASM
    elif format == "bin":
        return _OutputFormat.BIN
    else:
        raise Exception(f"Unrecognized output format: \"{format}\"")


def assemble(vcp_file, output_file, format_str):
    # Parse the file.
    statements = parse_vcp_file(vcp_file)

    # Translate the statements into code.
    code = translate_code(statements)

    # Generate the output file.
    format = get_format(format_str, output_file)
    if format == _OutputFormat.GNU_ASM:
        write_asm(output_file, code, vcp_file)
    elif format == _OutputFormat.BIN:
        write_bin(output_file, code)


def main():
    # Parse command line arguments.
    parser = argparse.ArgumentParser(
            description='MC1 Video Control Program (VCP) assembler')
    parser.add_argument('vcp', metavar='VCP_FILE', help='the VCP program to assemble')
    parser.add_argument('-o', '--output', required=True, help='the output file')
    parser.add_argument('-f', '--format', required=False, default="auto", help='the output format (auto, asm or bin)')
    args = parser.parse_args()

    # Assemble the file.
    assemble(args.vcp, args.output, args.format)


if __name__ == "__main__":
    main()
