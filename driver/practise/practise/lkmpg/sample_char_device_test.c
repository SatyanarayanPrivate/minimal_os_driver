#include<unistd.h>
#include<sys/types.h>
#include<linux/fcntl.h>
#include<stdio.h>
#include<stdlib.h>

int main (void) {
    
    int fd,ret;
    char buf[1024];
    fd = open("/dev/chardevice",O_RDONLY);
    if(fd<0){
        perror("error in opening");
        exit(1);
    }
    //lseek(fd,22,SEEK_SET);

    printf ("\ndevice opened");
    ret = read(fd,buf,60);
    printf("the no. characters returned is %d\n", ret);
    buf[ret] = 0x00;
    
    printf ("\nread Contents:: %s\n", buf);
    
    close (fd);
    
    
    return 0;
}