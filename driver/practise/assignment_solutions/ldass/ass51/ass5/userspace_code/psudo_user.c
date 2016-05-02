#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<errno.h>
#include<string.h>
#include<pthread.h>
#include<semaphore.h>
#include<string.h>
#include<fcntl.h>


static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
void *prod_func();
void *cons_func();
int ret,fd;

int main()
{
	
	
	int ret1,ret2;
	pthread_t producer,consumer;
	ret1=pthread_create(&producer,NULL,&cons_func,NULL);
	ret2=pthread_create(&consumer,NULL,&prod_func,NULL);
	
	if(ret1 != 0 || ret2 != 0){perror("Error in Creating Threads");		exit(1);}
	
	
	pthread_join(producer,NULL);
	pthread_join(consumer,NULL);
	
	
	return 0;
}
	
void *prod_func()
{

	char value[50];
	printf("In producer\n");
	pthread_mutex_lock(&mutex1);
	fd = open("/dev/our_char",O_RDONLY);
	ret = read(fd,&value,60);
	write(1,&value,ret);
	close(fd);
			
	pthread_mutex_unlock(&mutex1);
	pthread_exit(NULL);
}


void *cons_func()
{
	pthread_mutex_lock(&mutex1);
	printf("in consumer\n");
	char strc[60]="Success is a journey not a destination";
	fd = open("/dev/our_char",O_WRONLY);
	ret = write(fd,&strc,60);
	printf("writing complete %d chars\n",ret);
	close(fd);
		
	pthread_mutex_unlock(&mutex1);
	pthread_exit(NULL);
}










		






