obj-m := lcdDriverko.o

lcdDriverko-objs := lcdroutines.o devroutines.o lcdDriver.o

COMPFLAGS:= -Wall

all:
	make $(COMPFLAGS) -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make $(COMPFLAGS) -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
