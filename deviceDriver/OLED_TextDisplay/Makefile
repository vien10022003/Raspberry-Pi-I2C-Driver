obj-m += verticalScrollText.o
 
KDIR = /lib/modules/$(shell uname -r)/build
KBUILD_EXTRA_SYMBOLS := /home/pi/Desktop/driver-OLED-SSD-1306/Raspberry-Pi-I2C-Driver/deviceDriver/I2CClientDriver/Module.symvers

all:
    make -C $(KDIR) M=$(shell pwd) modules
 
clean:
    make -C $(KDIR) M=$(shell pwd) clean