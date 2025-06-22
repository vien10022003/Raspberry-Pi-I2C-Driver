obj-m += oled_text_display.o font5x7.o

PWD := $(shell pwd)
KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) modules

clean:
	make -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) clean

load:
	sudo insmod oled_text_display.ko

unload:
	sudo rmmod oled_text_display

install:
	sudo cp oled_text_display.ko /lib/modules/$(shell uname -r)/
	sudo depmod -a

help:
	@echo "make - Compile the driver"
	@echo "make clean - Clean the build files"
	@echo "make load - Load the driver"
	@echo "make unload - Unload the driver"
	@echo "make install - Install the driver"
	@echo "Usage after loading module:"
	@echo "  echo u > /proc/oled_control - Move selection up"
	@echo "  echo d > /proc/oled_control - Move selection down"