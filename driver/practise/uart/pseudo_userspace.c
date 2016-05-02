#include<unistd.h>
#include<sys/types.h>
#include<linux/fcntl.h>
#include<string.h>
#include <pthread.h>
#include <semaphore.h>

#include <errno.h>


static sem_t rdd;
static sem_t wrt;



int error;

#define QUERRY 1010
#define RESET 2020

pthread_t tid1, tid2;
static pthread_mutex_t  thread_buffer_lock   = PTHREAD_MUTEX_INITIALIZER;

void *thrdwr (int* arg);
void *thrdrd (int* arg);

int rdexit=1;

int main()
{

   int fd, fd2, fd3;
	int ret,buf21[4]={111, 222, 333, 444}, rd, wr=100;
	int *rrd, *wwr;

	
	wwr=&wr;
   fd = open("/dev/pseudo0",O_RDWR);
   if(fd<0){
           perror("error in opening");
           exit(1);
   }
  

   fd2 = open("Mydevicefileopen/lkd.pdf",O_RDONLY);
   if(fd2<0){
           perror("error in opening");
           exit(1);
   }


   fd3 = open("Mydevicefilewrite/mydoc.pdf",O_RDWR);
   if(fd3<0){
           perror("error in opening");
           exit(1);
   }


	ret=ioctl(fd, RESET, NULL);
	printf("the returned from reset is %d\n", ret);

	if (sem_init(&wrt, 0, 1))
    		return errno;
	if (sem_init(&rdd, 0, 0))
      		return errno;

	int a[2];

	a[0]=fd;
	a[1]=fd2;	
	pthread_create( &tid1, NULL, thrdwr, a);

	int b[2];
	b[0]=fd;
	b[1]=fd3;
	pthread_create( &tid2, NULL, thrdrd, b);




	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);


   exit(0);

}



void *thrdwr (int* arg)
{
	int fd, fd2, ret;
	char buf[100];
	fd=arg[0];
	fd2=arg[1];

	printf("In write thread \n");
	while(1){

//sem_wait(&wrt);
//pthread_mutex_lock(&thread_buffer_lock);
	ret = read(fd2, buf, 100);
	if(ret==0) 
	{
		printf("Read from file completed\n");
//		pthread_mutex_unlock(&thread_buffer_lock);
//		sem_post(&rdd);
		rdexit=0;
		break ;
	}
   
	printf("WR->the no. characters returned is from read file %d\n", ret);

	ret=write(fd, buf, ret);
	printf("WR->the no. characters written in device is %d\n", ret);

//pthread_mutex_unlock(&thread_buffer_lock);
//sem_post(&rdd);
   	}

	pthread_exit(0);
}



void *thrdrd (int* arg)
{

	int fd, fd3, ret;
	char buf[100];
	fd=arg[0];
	fd3=arg[1];
	
	
	printf("In read thread \n");
	do{
//sem_wait(&rdd);
//pthread_mutex_lock(&thread_buffer_lock);

	ret=ioctl(fd, QUERRY, NULL);
	printf("RD->the returned from ioctl is %d\n", ret);

	if(ret == 0 && rdexit == 0)
		break;

	ret = read(fd, buf, sizeof(buf));
	printf("RD->the no. characters returned after reading device is %d\n", ret);

	ret=write(fd3, buf, ret);
	printf("RD->the no. characters written in new file is %d\n", ret);

	
//pthread_mutex_unlock(&thread_buffer_lock);
//sem_post(&wrt);
	}while(1);


	pthread_exit(0);

}







