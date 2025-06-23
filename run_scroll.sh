#!/bin/bash

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}🎓 NHÓM 3 LỚP L01 - TEXT SCROLL DISPLAY${NC}"

# Cleanup
sudo rmmod scroll_doc 2>/dev/null || true
sudo rmmod I2CDriver 2>/dev/null || true

# Build I2C driver first
echo -e "${GREEN}Building I2C Driver...${NC}"
cd deviceDriver/I2CClientDriver
make clean && make

# Load I2C driver
echo -e "${GREEN}Loading I2C Driver...${NC}"
sudo insmod I2CDriver.ko

# Build scroll module
echo -e "${GREEN}Building Scroll Module...${NC}"
cd ../scrollText

echo "Cleaning first..."
make clean

echo "Copying Module.symvers AFTER clean..."
cp ../I2CClientDriver/Module.symvers ./Module.symvers

echo "Building with KBUILD_EXTRA_SYMBOLS..."
KBUILD_EXTRA_SYMBOLS=$(pwd)/Module.symvers make

# Load scroll module
echo -e "${GREEN}Loading Scroll Module...${NC}"
sudo insmod scroll_doc.ko

cd ../../

echo -e "${GREEN}✅ SUCCESS!${NC}"
dmesg | tail -5