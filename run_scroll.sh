#!/bin/bash

set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}🎓 NHÓM 3 LỚP L01 - TEXT SCROLL DISPLAY${NC}"

# Check if I2CDriver is loaded
if ! lsmod | grep -q "I2CDriver"; then
    echo -e "${YELLOW}⚠️  I2CDriver not loaded. Loading it first...${NC}"
    
    # Build I2C driver first
    echo -e "${GREEN}Building I2C Driver...${NC}"
    cd deviceDriver/I2CClientDriver
    make clean && make
    
    # Load I2C driver
    echo -e "${GREEN}Loading I2C Driver...${NC}"
    sudo insmod I2CDriver.ko
    cd ../../
else
    echo -e "${GREEN}✅ I2CDriver already loaded${NC}"
fi

# Cleanup previous scroll module
echo -e "${GREEN}Cleaning up previous scroll module...${NC}"
sudo rmmod scroll_doc 2>/dev/null || true

# Build scroll module
echo -e "${GREEN}Building Scroll Module...${NC}"
cd deviceDriver/scrollText

echo "Cleaning first..."
make clean

echo "Copying Module.symvers AFTER clean..."
cp ../I2CClientDriver/Module.symvers ./Module.symvers 2>/dev/null || {
    echo -e "${RED}❌ Cannot find Module.symvers from I2CClientDriver${NC}"
    echo -e "${YELLOW}Building I2CClientDriver first...${NC}"
    cd ../I2CClientDriver && make && cd ../scrollText
    cp ../I2CClientDriver/Module.symvers ./Module.symvers
}

echo "Building with KBUILD_EXTRA_SYMBOLS..."
KBUILD_EXTRA_SYMBOLS=$(pwd)/Module.symvers make

# Load scroll module
echo -e "${GREEN}Loading Scroll Module...${NC}"
sudo insmod scroll_doc.ko

cd ../../

echo -e "${GREEN}✅ SUCCESS!${NC}"
echo -e "${BLUE}📋 Recent kernel messages:${NC}"
dmesg | tail -5

echo -e "${YELLOW}🎮 Controls:${NC}"
echo -e "  SPACE     = Toggle auto scroll"
echo -e "  ESC       = Reset display & scroll"
echo -e "  UP/DOWN   = Manual scroll"
echo -e "  LEFT/RIGHT= Change speed"