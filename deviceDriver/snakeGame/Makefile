obj-m += snakeGame.o
 
KDIR = /lib/modules/$(shell uname -r)/build
KBUILD_EXTRA_SYMBOLS := /home/pi/Desktop/driver-OLED-SSD-1306/Raspberry-Pi-I2C-Driver/deviceDriver/I2CClientDriver/Module.symvers
# SYM_PATH := $(shell realpath ../I2CClientDriver/Module.symvers)
# KBUILD_EXTRA_SYMBOLS := $(SYM_PATH)

all:
	make -C $(KDIR)  M=$(shell pwd) modules
 
clean:
	make -C $(KDIR)  M=$(shell pwd) clean
