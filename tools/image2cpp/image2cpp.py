#!/usr/bin/env python3
"""
Created by ChatGPT from sources of https://github.com/javl/image2cpp
"""

import argparse
import os
import sys

from PIL import Image
import re


# ------------------------------------------------------------
# Utilities
# ------------------------------------------------------------

def swap_bits(byte):
    b = '{:08b}'.format(byte)
    return int(b[::-1], 2)


def threshold_pixel(pixel, threshold, invert):
    bit = 1 if pixel < threshold else 0
    if invert:
        bit ^= 1
    return bit


# ------------------------------------------------------------
# Bitmap conversion
# ------------------------------------------------------------

def horizontal_1bit(img, threshold, invert, swap):
    width, height = img.size
    pixels = img.load()

    data = []
    for y in range(height):
        byte = 0
        count = 0

        for x in range(width):
            bit = threshold_pixel(pixels[x, y], threshold, invert)
            byte = (byte << 1) | bit
            count += 1

            if count == 8:
                if swap:
                    byte = swap_bits(byte)
                data.append(byte)
                byte = 0
                count = 0

        if count != 0:
            byte <<= (8 - count)
            if swap:
                byte = swap_bits(byte)
            data.append(byte)

    return data


def vertical_1bit(img, threshold, invert, swap):
    width, height = img.size
    pixels = img.load()

    data = []

    for x in range(width):
        byte = 0
        count = 0

        for y in range(height):
            bit = threshold_pixel(pixels[x, y], threshold, invert)
            byte = (byte << 1) | bit
            count += 1

            if count == 8:
                if swap:
                    byte = swap_bits(byte)
                data.append(byte)
                byte = 0
                count = 0

        if count != 0:
            byte <<= (8 - count)
            if swap:
                byte = swap_bits(byte)
            data.append(byte)

    return data


# ------------------------------------------------------------
# Output formatting
# ------------------------------------------------------------

def format_plain(data):
    return ", ".join(f"0x{b:02X}" for b in data)


def format_arduino(name, width, height, data):
    lines = [
        f"// '{name}', {width}x{height}px",
        f"const unsigned char {name}[] PROGMEM = {{"
    ]

    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append("  " + line + ",")

    lines[-1] = lines[-1].rstrip(",")
    lines.append("};")
    return "\n".join(lines)


def format_arduino_single(name, width, height, data):
    body = ", ".join(f"0x{b:02X}" for b in data)
    return f"const unsigned char {name}[] PROGMEM = {{{body}}};"


def format_adafruit_gfx(name, width, height, data):
    body = ", ".join(f"0x{b:02X}" for b in data)
    return f"""
#define {name}_width {width}
#define {name}_height {height}

static const unsigned char {name}_data[] PROGMEM = {{
{body}
}};
"""


# ------------------------------------------------------------
# Conversion driver
# ------------------------------------------------------------

def convert_image(args):

    if not os.path.exists(args.input):
        print("Input file not found")
        sys.exit(1)

    img = Image.open(args.input).convert("L")

    width, height = img.size

    if args.mode == "horizontal1bit":
        data = horizontal_1bit(img, args.threshold, args.invert, args.swap)

    elif args.mode == "vertical1bit":
        data = vertical_1bit(img, args.threshold, args.invert, args.swap)

    else:
        raise ValueError("Only 1-bit modes implemented in this Python version.")

    name = args.name or os.path.splitext(os.path.basename(args.input))[0]

    # Replace all non-alphanumeric chars in name with underscore
    name = re.sub(r'\W', '_', name)

    if args.format == "plain":
        output = format_plain(data)
    elif args.format == "arduino":
        output = format_arduino(name, width, height, data)
    elif args.format == "arduino_single":
        output = format_arduino_single(name, width, height, data)
    elif args.format == "adafruit_gfx":
        output = format_adafruit_gfx(name, width, height, data)
    else:
        raise ValueError("Unknown format")

    return output


# ------------------------------------------------------------
# CLI
# ------------------------------------------------------------

def main():

    parser = argparse.ArgumentParser(description="image2cpp Python CLI")

    parser.add_argument("-i", "--input", required=True,
                        help="input image (png/jpg/bmp)")

    parser.add_argument("-o", "--output",
                        help="output file")

    parser.add_argument("-n", "--name",
                        help="variable name")

    parser.add_argument("-f", "--format",
                        default="arduino",
                        choices=["plain", "arduino", "arduino_single", "adafruit_gfx"],
                        help="output format")

    parser.add_argument("-m", "--mode",
                        default="horizontal1bit",
                        choices=["horizontal1bit", "vertical1bit"],
                        help="draw mode")

    parser.add_argument("-t", "--threshold",
                        type=int,
                        default=128,
                        help="brightness threshold")

    parser.add_argument("-x", "--invert",
                        action="store_true",
                        help="invert colors")

    parser.add_argument("-s", "--swap",
                        action="store_true",
                        help="swap bits in byte")

    args = parser.parse_args()

    result = convert_image(args)

    if args.output:
        with open(args.output, "w") as f:
            f.write(result)
    else:
        print(result)


if __name__ == "__main__":
    main()
