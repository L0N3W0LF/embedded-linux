obj-m += my_gpio.o

GCC=/opt/fontys/soekris/bin/i586-linux-
SRC=~/fontys-soekris-linux-2.6.35.7/

all:
	make ARCH=i386 CROSS_COMPILE=$(GCC) -C $(SRC) M=$(PWD) modules
clean:
	make ARCH=i386 CROSS_COMPILE=$(GCC) -C $(SRC) M=$(PWD) clean