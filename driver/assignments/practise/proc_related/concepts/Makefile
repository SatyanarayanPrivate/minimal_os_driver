obj-m	+= simple_proc_dir.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc simple_proc_dir_test.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm *.out