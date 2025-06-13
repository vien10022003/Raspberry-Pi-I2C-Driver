#!/bin/bash

set -e  # Dừng nếu có lỗi

# Màu terminal
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo -e "${GREEN}🧹 Removing snakeGame module...${NC}"
sudo rmmod snakeGame || echo "⚠️  Không thể gỡ snakeGame (có thể chưa được nạp)"

echo -e "${GREEN}🧹 Removing I2CDriver module...${NC}"
sudo rmmod I2CDriver || echo "⚠️  Không thể gỡ I2CDriver (có thể chưa được nạp)"

echo -e "${GREEN}✅ All modules removed successfully (nếu đã được nạp).${NC}"
