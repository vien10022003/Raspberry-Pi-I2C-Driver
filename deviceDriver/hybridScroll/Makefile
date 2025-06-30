obj-m += hybridScroll.o

KDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

# Include symbols from I2CDriver
EXTRA_SYMBOLS = $(PWD)/../I2CClientDriver/Module.symvers

all:
	@if [ -f "$(EXTRA_SYMBOLS)" ]; then \
		cp $(EXTRA_SYMBOLS) $(PWD)/; \
	fi
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f Module.symvers 