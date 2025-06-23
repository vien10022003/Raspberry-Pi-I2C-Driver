#!/bin/bash

set -e  # Dừng script nếu có lỗi xảy ra

# Màu cho terminal
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}🎓 NHÓM 3 LỚP L01 - TEXT SCROLL DISPLAY${NC}"
echo -e "${YELLOW}Thành viên: Tô Quang Viễn, Bùi Đức Khánh, Nguyễn Thị Hồng Ngân, Thân Nhân Chính${NC}"
echo ""

echo -e "${GREEN}🧹 Building I2C Driver...${NC}"
cd deviceDriver/I2CClientDriver
make clean
make

echo -e "${GREEN}📦 Loading I2C Driver...${NC}"
sudo insmod I2CDriver.ko || {
    echo "⚠️  Module I2CDriver có thể đã được nạp. Đang thử remove và load lại..."
    sudo rmmod I2CDriver
    sudo insmod I2CDriver.ko
}

cd - > /dev/null

echo -e "${GREEN}🧹 Building Scroll Text Module...${NC}"
cd deviceDriver/scrollText
make clean
make

echo -e "${GREEN}📜 Loading Scroll Text Module...${NC}"
sudo insmod scroll_doc.ko || {
    echo "⚠️  Module scroll_doc có thể đã được nạp. Đang thử remove và load lại..."
    sudo rmmod scroll_doc
    sudo insmod scroll_doc.ko
}

cd - > /dev/null

echo -e "${GREEN}✅ Scroll text module loaded successfully!${NC}"
echo ""
echo -e "${YELLOW}🎮 Controls:${NC}"
echo -e "  ⬆️  UP/DOWN: Manual scroll"
echo -e "  ⬅️  LEFT/RIGHT: Speed control"  
echo -e "  ⏸️  SPACE: Toggle auto scroll"
echo -e "  🔄 ESC: Reset position"
echo ""
echo -e "${YELLOW}📋 To view kernel logs: dmesg | tail${NC}"
echo -e "${YELLOW}🛑 To stop: sudo ./rmmod_scroll.sh${NC}" 