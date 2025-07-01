#!/bin/bash
echo 1 | sudo tee /sys/kernel/oled_scroll/direction
echo 1 | sudo tee /sys/kernel/oled_scroll/enable
