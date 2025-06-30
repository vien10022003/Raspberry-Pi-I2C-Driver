#!/bin/bash

set -e  # Dừng nếu có lỗi

# Màu terminal
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}🧹 Removing scroll text module safely...${NC}"

# Kiểm tra xem module có đang chạy không
if lsmod | grep -q "scroll_doc"; then
    echo -e "${YELLOW}📋 Module scroll_doc is loaded, removing...${NC}"
    sudo rmmod scroll_doc
    echo -e "${GREEN}✅ scroll_doc removed${NC}"
else
    echo -e "${YELLOW}⚠️  Module scroll_doc not loaded${NC}"
fi

# Chờ một chút để system cleanup
sleep 1

echo -e "${GREEN}🔍 Checking module status...${NC}"
lsmod | grep -E "(scroll_doc|I2CDriver)" || echo "No scroll modules found"

echo -e "${GREEN}✅ Scroll module cleanup completed!${NC}" 