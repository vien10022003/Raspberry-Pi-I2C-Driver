#!/bin/bash

set -e  # Dừng nếu có lỗi

# Màu terminal
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo -e "${GREEN}🧹 Removing scroll text module...${NC}"
sudo rmmod scroll_doc || echo "⚠️  Không thể gỡ scroll_doc (có thể chưa được nạp)"

echo -e "${GREEN}🧹 Removing I2C Driver module...${NC}"
sudo rmmod I2CDriver || echo "⚠️  Không thể gỡ I2CDriver (có thể chưa được nạp)"

echo -e "${GREEN}✅ All modules removed successfully!${NC}" 