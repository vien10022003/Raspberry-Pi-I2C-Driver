obj-m += combined_scroll.o

KERNEL_PATH = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean

install:
	sudo insmod combined_scroll.ko

remove:
	sudo rmmod combined_scroll

.PHONY: all clean install remove
