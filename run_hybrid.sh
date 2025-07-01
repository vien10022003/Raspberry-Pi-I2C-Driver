#!/bin/bash

# Script to run hybrid scroll module

echo "=== HYBRID SCROLL MODULE LOADER ==="
echo "Building and loading hybrid scroll module..."

# Build I2C driver trước
echo "1. Building I2C Client Driver..."
cd deviceDriver/I2CClientDriver
make clean && make
if [ $? -ne 0 ]; then
    echo "Failed to build I2CClientDriver"
    exit 1
fi

echo "2. Loading I2C Client Driver..."
sudo insmod I2CDriver.ko
if [ $? -ne 0 ]; then
    echo "Failed to load I2CClientDriver"
    exit 1
fi

# Kiểm tra xem Module.symvers có được tạo không
echo "Checking for Module.symvers..."
if [ -f "Module.symvers" ]; then
    echo "✓ Module.symvers found at $(pwd)/Module.symvers"
    echo "Symbol contents:"
    cat Module.symvers
else
    echo "✗ Module.symvers not found in I2CClientDriver directory"
    echo "Generated files:"
    ls -la *.symvers *.ko 2>/dev/null || echo "No .symvers or .ko files found"
    exit 1
fi

# Chờ một chút để module khởi tạo
sleep 2

# Build hybrid scroll module
cd ../hybridScroll
echo "3. Building Hybrid Scroll Module..."
make clean && make
if [ $? -ne 0 ]; then
    echo "Failed to build HybridScroll"
    echo "Debugging info:"
    echo "Current directory: $(pwd)"
    echo "Makefile contents:"
    cat Makefile
    exit 1
fi

echo "4. Loading Hybrid Scroll Module..."
sudo insmod hybridScroll.ko
if [ $? -ne 0 ]; then
    echo "Failed to load HybridScroll"
    echo "Module may have dependency issues. Check dmesg for details."
    sudo dmesg | tail -10
    exit 1
fi

echo "=== SUCCESS! ==="
echo "Hybrid Scroll Module is now running!"
echo ""
echo "Controls:"
echo "  - Arrow keys: Manual scroll (UP/DOWN = vertical, LEFT/RIGHT = horizontal)"
echo "  - SPACE: Toggle horizontal auto scroll"
echo "  - Z: Toggle vertical auto scroll"  
echo "  - ESC: Reset all scroll positions"
echo "  - Q: Demo mode (auto scroll both directions)"
echo "  - P: Pause all auto scroll"
echo ""
echo "To view kernel messages: sudo dmesg | tail"
echo "To stop: sudo ./rmmod_hybrid.sh"

cd ../.. 