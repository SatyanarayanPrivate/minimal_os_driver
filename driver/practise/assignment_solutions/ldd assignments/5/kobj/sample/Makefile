obj-m := prog1.o

default:
	make -C /usr/src/linux-2.6.34-12 M=`pwd` modules
clean:
	make -C /usr/src/linux-2.6.34-12 M=`pwd` clean
