#!/bin/bash

echo "Loading OLED Text Display modules..."

# Remove existing modules if loaded (ignore errors if not loaded)
echo "Removing existing modules..."
sudo rmmod verticalScrollText 2>/dev/null || true
sudo rmmod I2CDriver 2>/dev/null || true

# Clean build directories
echo "Cleaning build directories..."
make clean -C deviceDriver/I2CClientDriver
make clean -C deviceDriver/OLED_TextDisplay

# Build I2C driver module
echo "Building I2C driver..."
make -C deviceDriver/I2CClientDriver

# Build vertical scroll text module  
echo "Building vertical scroll text module..."
make -C deviceDriver/OLED_TextDisplay

# Load I2C driver module
echo "Loading I2C driver..."
sudo insmod deviceDriver/I2CClientDriver/I2CDriver.ko

# Load vertical scroll text module
echo "Loading vertical scroll text module..."
sudo insmod deviceDriver/OLED_TextDisplay/verticalScrollText.ko

echo "OLED Text Display modules loaded successfully!"
echo ""

# Function to display menu
display_menu() {
    echo "========================================="
    echo "        OLED SCROLL CONTROL MENU"
    echo "========================================="
    echo "Manual Controls:"
    echo "  u  - Scroll up one line"
    echo "  d  - Scroll down one line"
    echo "  s  - Shift selected line left"
    echo ""
    echo "Auto Controls:"
    echo "  ad - Enable auto scroll down"
    echo "  Ad - Disable auto scroll down"
    echo "  as - Enable auto horizontal scroll"
    echo "  As - Disable auto horizontal scroll"
    echo ""
    echo "  0  - Exit program"
    echo "========================================="
    echo -n "Enter your choice: "
}

# Main interactive loop
while true; do
    display_menu
    read choice
    
    case $choice in
        "u")
            echo "Scrolling up one line..."
            echo up | sudo tee /sys/kernel/oled_scroll/scroll > /dev/null
            echo "✓ Scrolled up successfully!"
            ;;
        "d")
            echo "Scrolling down one line..."
            echo down | sudo tee /sys/kernel/oled_scroll/scroll > /dev/null
            echo "✓ Scrolled down successfully!"
            ;;
        "s")
            echo "Shifting selected line left..."
            echo 1 | sudo tee /sys/kernel/oled_scroll/horizontal_shift > /dev/null
            echo "✓ Line shifted left successfully!"
            ;;
        "ad")
            echo "Enabling auto scroll down..."
            echo 1 | sudo tee /sys/kernel/oled_scroll/direction > /dev/null
            echo 1 | sudo tee /sys/kernel/oled_scroll/enable > /dev/null
            echo "✓ Auto scroll down enabled!"
            ;;
        "Ad")
            echo "Disabling auto scroll down..."
            echo 0 | sudo tee /sys/kernel/oled_scroll/enable > /dev/null
            echo "✓ Auto scroll down disabled!"
            ;;
        "as")
            echo "Enabling auto horizontal scroll..."
            echo 1 | sudo tee /sys/kernel/oled_scroll/horizontal_auto > /dev/null
            echo "✓ Auto horizontal scroll enabled!"
            ;;
        "As")
            echo "Disabling auto horizontal scroll..."
            echo 0 | sudo tee /sys/kernel/oled_scroll/horizontal_auto > /dev/null
            echo "✓ Auto horizontal scroll disabled!"
            ;;
        "0")
            echo "Exiting OLED control program..."
            echo "Unloading modules..."
            sudo rmmod verticalScrollText
            sudo rmmod I2CDriver
            echo "Goodbye!"
            exit 0
            ;;
        *)
            echo "❌ Invalid choice: '$choice'"
            echo "Please enter a valid option (u, d, s, ad, Ad, as, As, or 0)"
            ;;
    esac
    
    echo ""
done
