#!/bin/bash

# Script to run hybrid scroll module

echo "=== HYBRID SCROLL MODULE LOADER ==="
echo "Building and loading hybrid scroll module..."

# Build and load I2C driver first
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

# Wait a moment for the module to initialize
sleep 1

# Build hybrid scroll module (after I2C driver is loaded and Module.symvers is created)
cd ../hybridScroll
echo "3. Building Hybrid Scroll Module..."
make clean && make
if [ $? -ne 0 ]; then
    echo "Failed to build HybridScroll"
    echo "Checking if I2CDriver symbols are available..."
    if [ ! -f "../I2CClientDriver/Module.symvers" ]; then
        echo "Module.symvers not found in I2CClientDriver directory"
        echo "Trying to rebuild I2CClientDriver..."
        cd ../I2CClientDriver
        make
        cd ../hybridScroll
        make
    fi
    if [ $? -ne 0 ]; then
        echo "Still failed to build. Exiting."
        exit 1
    fi
fi

echo "4. Loading Hybrid Scroll Module..."
sudo insmod hybridScroll.ko
if [ $? -ne 0 ]; then
    echo "Failed to load HybridScroll"
    echo "Module may have dependency issues. Check dmesg for details."
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