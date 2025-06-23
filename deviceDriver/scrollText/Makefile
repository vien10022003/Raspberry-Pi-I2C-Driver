obj-m += scroll_doc.o

KDIR = /lib/modules/$(shell uname -r)/build
KBUILD_EXTRA_SYMBOLS = ../I2CClientDriver/Module.symvers

all:
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean