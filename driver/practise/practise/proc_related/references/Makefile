obj-m	+= proc_seq_example1.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc proc_seq_test.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm *.out