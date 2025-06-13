# Raspberry-Pi-I2C-Driver

This project contains two kernel modules developed for the Raspberry Pi:

- I2CClientDriver: A simple I2C client driver for SSD1306 OLED display.
- snakeGame: A kernel module (e.g., for keyboard input or game logic).

## Project Structure
```
deviceDriver/
├── I2CClientDriver/       # Contains I2C driver code for SSD1306 OLED
│   ├── I2CDriver.ko
│   ├── driver.c
│   └── Makefile
├── snakeGame/             # Contains second kernel module (e.g., using keyboard)
│   ├── snakeGame.ko
│   ├── snake.c
│   └── Makefile
insmod.sh                  # Script to build and load both modules
rmmod.sh                   # Script to unload both modules
README.md                  # This file
```
## Requirements

- Raspberry Pi running Linux (tested with kernel 5.x)
- make, gcc, and kernel headers installed:

  sudo apt install build-essential raspberrypi-kernel-headers

## Build & Load Modules

### Step 1: Compile and Insert Modules

```
  sudo ./insmod.sh
```

This will:
- Clean and build I2CClientDriver
- Insert I2CDriver.ko into the kernel
- Clean and build snakeGame
- Insert snakeGame.ko into the kernel

### Step 2: Remove Modules

```
  sudo ./rmmod.sh
```

This will unload both modules safely.

## Testing

- After inserting the I2CClientDriver, the OLED screen should initialize and display data (e.g., fill screen).
- The snakeGame module may interact with keyboard events or be extended to do so (e.g., with keyboard_notifier).

## Extending

You can modify:
- I2CClientDriver/driver.c: Add custom text rendering to the OLED.
- snakeGame/snake.c: Add logic to handle keyboard input and send data to OLED driver via exported functions.

## Author

- Based on tutorial code by EmbeTronicX (https://embetronicx.com/)
- Extended and modified for Raspberry Pi kernel development practice.

## License

GPLv2 — free to use and modify under GNU Public License.
