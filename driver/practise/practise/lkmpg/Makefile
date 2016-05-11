# simple module compile
# obj-m += hello_word_1.o

# compile multiple modules inn single make file
# obj-m += hello_word_1.o hello_word_2.o
# above and below one and the same
# obj-m += hello_word_1.o
# obj-m += hello_word_2.o

# Module parameter
obj-m += module_param.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


