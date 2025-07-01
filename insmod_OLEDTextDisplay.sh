#!/bin/bash

echo "Loading OLED Text Display modules..."

# Load I2C driver module
echo "Loading I2C driver..."
sudo insmod deviceDriver/I2CClientDriver/I2CDriver.ko

# Load vertical scroll text module
echo "Loading vertical scroll text module..."
sudo insmod deviceDriver/OLED_TextDisplay/verticalScrollText.ko

# Grant execute permissions to all shell scripts in OLED_TextDisplay directory
echo "Setting execute permissions for shell scripts..."
chmod +x deviceDriver/OLED_TextDisplay/*.sh

echo "OLED Text Display modules loaded successfully!"
echo "Available sysfs controls:"
echo "  /sys/kernel/oled_scroll/scroll"
echo "  /sys/kernel/oled_scroll/horizontal_shift"
echo "  /sys/kernel/oled_scroll/horizontal_auto"
echo "  /sys/kernel/oled_scroll/enable"
echo "  /sys/kernel/oled_scroll/direction"
echo ""
echo "Available shell scripts in deviceDriver/OLED_TextDisplay/:"
echo "  ./up.sh - Scroll up one line"
echo "  ./down.sh - Scroll down one line"
echo "  ./shift.sh - Shift selected line left"
echo "  ./autoShift.sh - Enable auto horizontal scroll"
echo "  ./stopAutoShift.sh - Disable auto horizontal scroll"
echo "  ./autoDown.sh - Enable auto vertical scroll down"
echo "  ./stopAutoDown.sh - Disable auto vertical scroll"
