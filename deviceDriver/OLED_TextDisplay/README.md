# OLED Text Display Driver for SSD1306

This Linux kernel module displays multiple lines of text on an SSD1306 OLED display connected via I2C. The driver provides features for navigating between text lines and automatic horizontal scrolling for long texts.

## Features

- Displays six configurable text lines on the OLED display
- Line navigation using /proc interface
- Automatic horizontal scrolling for lines that exceed the display width
- Visual indication of the currently selected line with "..." at the end (for short lines)

## Hardware Requirements

- Raspberry Pi or compatible board
- SSD1306 OLED Display (128x64 pixels)
- I2C connection to the OLED display

## Lines Displayed

The driver will display the following fixed text lines:

1. "Driver - L01"
2. "NHOM 5"
3. "Thanh vien 1: Bui Duc Khanh CT060119"
4. "Thanh vien 2: Bui Duc Khanh CT060119"
5. "Thanh vien 3: Bui Duc Khanh CT060119"
6. "Bui Duc Khanh CT060119"

## Installation

1. Compile the driver:

```
make
```

2. Load the driver:

```
make load
```

3. Verify the driver is loaded:

```
dmesg | grep OLED
```

4. To unload the driver:

```
make unload
```

## Usage

After loading the driver, use the following commands to navigate between lines:

- To move the selection up:

```
echo u > /proc/oled_control
```

- To move the selection down:

```
echo d > /proc/oled_control
```

## How It Works

- When the driver is loaded, it initializes the SSD1306 OLED display and shows the first line
- The currently selected line will have "..." at the end (if it's short enough)
- Long lines will automatically scroll horizontally to display all content
- You can navigate between lines using the /proc interface

## Files

- `oled_text_display.c` - Main driver code
- `font5x7.c` - 5x7 pixel font data for text display
- `Makefile` - For building the driver
- `README.md` - Documentation (this file)

## Author

NHOM 5

## License

GPL
