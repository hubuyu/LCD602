obj-m = lcd.o
KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod.c *.order *.symvers	
