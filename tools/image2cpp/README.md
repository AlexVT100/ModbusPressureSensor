# Image to CPP Converter

Created by ChatGPT from the sources of https://github.com/javl/image2cpp

Below is a complete Python CLI tool that closely replicates the behavior of image2cpp.
It supports the same important options used in CLI ports of the project: format selection, draw mode, threshold, inversion, and bit swapping. The original tool converts images into byte arrays for microcontroller displays such as Arduino OLEDs.

This version aims to match the typical command-line usage found in community CLI conversions.

## Install dependencies

```shell
pip install pillow
```

## Example usage (same style as image2cpp CLI)

### Convert image → Arduino bitmap

```shell
python image2cpp_cli.py \
 -i logo.png \
 -o logo.c \
 -f arduino \
 -m horizontal1bit \
 -n logoBitmap
```

### Invert image

```shell
python image2cpp_cli.py -i icon.png -x
```

### Vertical byte layout

```shell
python image2cpp_cli.py -i icon.png -m vertical1bit
```

### Swap bits (useful for U8g2)

```shell
python image2cpp_cli.py -i icon.png -s
```

### Output example

```cpp
// 'logo', 128x64px
const unsigned char logo[] PROGMEM = {
  0x00, 0xFF, 0x81, 0x81, ...
};
```

## Features implemented

- CLI arguments compatible with typical image2cpp usage
- Horizontal / vertical 1-bit modes
- Arduino / plain / Adafruit GFX output
- Bit swapping
- Inversion
- Threshold control
