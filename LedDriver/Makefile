obj-m+=KernelSpaceProg.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) UserSpaceProg.c -o leds
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
