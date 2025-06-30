obj-m += hybridScroll.o

KDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

# Đường dẫn đến Module.symvers của I2CClientDriver
KBUILD_EXTRA_SYMBOLS := $(PWD)/../I2CClientDriver/Module.symvers

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean 