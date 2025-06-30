#!/bin/bash

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}🧪 TESTING SCROLL MODULE${NC}"

echo -e "${GREEN}=== Module Status ===${NC}"
if lsmod | grep -q "scroll_doc"; then
    echo "✅ scroll_doc: LOADED"
else
    echo "❌ scroll_doc: NOT LOADED"
fi

if lsmod | grep -q "I2CDriver"; then
    echo "✅ I2CDriver: LOADED"
else
    echo "❌ I2CDriver: NOT LOADED"
fi

echo -e "${GREEN}=== I2C Device Check ===${NC}"
if sudo i2cdetect -y 1 | grep -q "3c\|UU"; then
    echo "✅ OLED found at address 0x3C"
else
    echo "❌ OLED not detected"
fi

echo -e "${GREEN}=== Recent Messages ===${NC}"
dmesg | grep -E "(scroll_doc|SSD1306|OLED)" | tail -5

echo -e "${GREEN}=== Test Instructions ===${NC}"
echo "1. Nhấn ESC để test reset display"
echo "2. Nhấn SPACE để toggle scroll" 
echo "3. Nhấn LEFT/RIGHT để test speed control"
echo "4. Chạy './rmmod_scroll.sh' để test cleanup"
