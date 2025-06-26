#!/bin/bash

echo "🗑️  REMOVING BIG SIZE SCROLL MODULES"

# Remove big scroll module
if lsmod | grep -q "scroll_doc"; then
    echo "Removing BIG scroll module..."
    sudo rmmod scroll_doc
    
    if [ $? -eq 0 ]; then
        echo "✅ BIG scroll module removed"
    else
        echo "⚠️  Failed to remove BIG scroll module"
    fi
else
    echo "ℹ️  BIG scroll module not loaded"
fi

# Remove I2CDriver (if no other modules using it)
if lsmod | grep -q "I2CDriver"; then
    echo "Removing I2CDriver..."
    sudo rmmod I2CDriver
    
    if [ $? -eq 0 ]; then
        echo "✅ I2CDriver removed"
    else
        echo "⚠️  Failed to remove I2CDriver (may be in use by other modules)"
    fi
else
    echo "ℹ️  I2CDriver not loaded"
fi

# Show final status
echo ""
echo "📋 Final module status:"
lsmod | grep -E "(scroll_doc|I2CDriver)" || echo "No scroll modules loaded"

echo ""
echo "✅ Cleanup completed!"