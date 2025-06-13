#!/bin/bash

set -e  # Dừng script nếu có lỗi xảy ra

# Màu cho terminal
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo -e "${GREEN}🧹 Cleaning and Building I2CClientDriver...${NC}"
cd deviceDriver/I2CClientDriver
make clean
make

echo -e "${GREEN}📦 Inserting I2CClientDriver module...${NC}"
sudo insmod I2CDriver.ko || {
    echo "⚠️  Module I2CDriver có thể đã được nạp. Đang thử remove và load lại..."
    sudo rmmod I2CDriver
    sudo insmod I2CDriver.ko
}

cd - > /dev/null

echo -e "${GREEN}🧹 Cleaning and Building snakeGame...${NC}"
cd deviceDriver/snakeGame
make clean
make

echo -e "${GREEN}🎮 Inserting snakeGame module...${NC}"
sudo insmod snakeGame.ko || {
    echo "⚠️  Module snakeGame có thể đã được nạp. Đang thử remove và load lại..."
    sudo rmmod snakeGame
    sudo insmod snakeGame.ko
}

cd - > /dev/null

echo -e "${GREEN}✅ All modules inserted successfully!${NC}"
