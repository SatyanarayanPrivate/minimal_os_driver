#include<unistd.h>
#include<sys/types.h>
#include<linux/fcntl.h>


int main()
{

   int fd,ret;
	char buf[1024];
//   fd = open("/sys/kernel/kset_devices_typeA/device0/dev_param1",O_RDWR);
	fd = open("/dev/pseudo0",O_RDWR);
   if(fd<0){
           perror("error in opening");
           exit(1);
   }
 
	int a=300, b=20;

	char wrp[10]={"Hello"}, rdp[10]={"Hi"};


printf("the characters writeen is %d \n", a);
	ret=write(fd, wrp, strlen(wrp));
printf("the no. characters writeen is %d \n", ret);


//	 ret = read(fd, rdp, ret);

//   printf("the no. characters returned is %d val %s\n", ret, rdp);

 

 
   exit(0);

}
