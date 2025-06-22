#!/bin/bash
# test_oled.sh - Simple script to test OLED text display driver

# Check if running as root
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 
   exit 1
fi

# Function to display help
show_help() {
  echo "OLED Display Controller"
  echo "----------------------"
  echo "Usage: $0 [option]"
  echo "Options:"
  echo "  up      - Move selection up"
  echo "  down    - Move selection down"
  echo "  watch   - Enter interactive mode (press q to quit, arrow keys to navigate)"
  echo "  load    - Load the driver"
  echo "  unload  - Unload the driver"
  echo "  help    - Show this help"
}

# Check if driver is loaded
check_driver() {
  lsmod | grep oled_text_display > /dev/null
  if [ $? -ne 0 ]; then
    echo "OLED driver is not loaded. Load it first with: $0 load"
    exit 1
  fi
  
  # Check if proc file exists
  if [ ! -f /proc/oled_control ]; then
    echo "OLED control interface not found at /proc/oled_control"
    exit 1
  fi
}

# Process commands
case "$1" in
  up)
    check_driver
    echo "Moving selection up"
    echo "u" > /proc/oled_control
    ;;
  down)
    check_driver
    echo "Moving selection down"
    echo "d" > /proc/oled_control
    ;;
  watch)
    check_driver
    clear
    echo "Interactive mode - use up/down arrow keys to navigate (press 'q' to quit)"
    # Use stty to get raw key inputs
    old_tty=$(stty -g)
    stty raw -echo
    while true; do
      key=$(dd bs=1 count=1 2>/dev/null)
      if [[ $key = q ]]; then
        break
      elif [[ $key = $'\e' ]]; then
        # This is an escape sequence
        read -rsn2 key
        if [[ $key = '[A' ]]; then
          # Up arrow
          echo "u" > /proc/oled_control
          echo -e "\rUp   "
        elif [[ $key = '[B' ]]; then
          # Down arrow
          echo "d" > /proc/oled_control
          echo -e "\rDown "
        fi
      fi
    done
    # Restore terminal settings
    stty $old_tty
    clear
    echo "Exited interactive mode"
    ;;
  load)
    echo "Loading OLED driver"
    insmod oled_text_display.ko
    dmesg | tail -3
    ;;
  unload)
    echo "Unloading OLED driver"
    rmmod oled_text_display
    ;;
  help|*)
    show_help
    ;;
esac

exit 0