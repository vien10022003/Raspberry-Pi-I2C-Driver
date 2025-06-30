#!/bin/bash

echo "🎓📝 NHÓM 3 LỚP L01 - BIG SIZE TEXT SCROLL DISPLAY"

# Check if I2CDriver is loaded
if ! lsmod | grep -q "I2CDriver"; then
    echo "⚠️  I2CDriver not loaded. Loading it first..."
    
    # Build and load I2CDriver
    echo "Building I2C Driver..."
    cd deviceDriver/I2CClientDriver
    make clean
    make
    
    if [ $? -ne 0 ]; then
        echo "❌ Failed to build I2CDriver"
        exit 1
    fi
    
    echo "Loading I2C Driver..."
    sudo insmod I2CDriver.ko
    
    if [ $? -ne 0 ]; then
        echo "❌ Failed to load I2CDriver"
        exit 1
    fi
    
    cd ../..
    
    # Wait for I2C driver to initialize
    sleep 2
else
    echo "✅ I2CDriver already loaded"
fi

# Clean up any existing big scroll module
echo "Cleaning up previous BIG scroll module..."
if lsmod | grep -q "scroll_doc"; then
    sudo rmmod scroll_doc 2>/dev/null
fi

# Build big scroll module
echo "Building BIG Size Scroll Module..."
cd deviceDriver/scrollTextBigSize

echo "Cleaning first..."
make clean

# Copy Module.symvers from I2CDriver to resolve symbols
echo "Copying Module.symvers AFTER clean..."
cp ../I2CClientDriver/Module.symvers .

echo "Building with KBUILD_EXTRA_SYMBOLS..."
export KBUILD_EXTRA_SYMBOLS=$(realpath ../I2CClientDriver/Module.symvers)
make

if [ $? -ne 0 ]; then
    echo "❌ Failed to build BIG scroll module"
    exit 1
fi

cd ../..

# Load big scroll module
echo "Loading BIG Size Scroll Module..."
sudo insmod deviceDriver/scrollTextBigSize/scroll_doc.ko

if [ $? -ne 0 ]; then
    echo "❌ Failed to load BIG scroll module"
    exit 1
fi

echo "✅ SUCCESS!"

# Show status
echo "📋 Recent kernel messages:"
dmesg | tail -10

echo ""
echo "🎮 Controls:"
echo "  SPACE     = Toggle auto scroll"
echo "  ESC       = Reset display & scroll"
echo "  UP/DOWN   = Manual scroll"
echo "  LEFT/RIGHT= Change speed"
echo "  Q         = Test BIG characters"
echo ""
echo "ℹ️  Use './rm_txtbigsize.sh' to remove modules"