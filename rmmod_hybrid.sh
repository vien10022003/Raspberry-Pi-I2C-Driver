#!/bin/bash

# Script to remove hybrid scroll module

echo "=== REMOVING HYBRID SCROLL MODULE ==="

echo "1. Removing Hybrid Scroll Module..."
sudo rmmod hybridScroll
if [ $? -eq 0 ]; then
    echo "HybridScroll module removed successfully"
else
    echo "Failed to remove HybridScroll module (may not be loaded)"
fi

echo "2. Removing I2C Client Driver..."
sudo rmmod I2CDriver
if [ $? -eq 0 ]; then
    echo "I2CDriver module removed successfully"
else
    echo "Failed to remove I2CDriver module (may not be loaded)"
fi

echo "=== CLEANUP COMPLETED ==="
echo "All modules removed!" 