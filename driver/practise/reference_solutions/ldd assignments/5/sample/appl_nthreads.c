#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<pthread.h>

#define SIZE 5
#define BUF_SIZE 1024
void* fun_write(void* pv);
void* fun_read(void* pv);



int main()
{   
	int i =0, choice =1, no_device, j;
	char *str[SIZE];
	int fdd[SIZE], *ptr_fd[SIZE];
	pthread_t tread[SIZE], twrite[SIZE], tself ;
	fdd[0] = 0;
	printf("Enter no of devices created \n");
	scanf("%d", &no_device);
	for(j=1; j<= no_device; j++)
	{	str[j] = (char*)malloc(BUF_SIZE*sizeof(char));
		printf("Enter the path of file\n");
		scanf("%s",str[j]);
		
		fdd[j] = open(str[j], O_RDWR);
		printf("fdd = %d\n",fdd[j]);
		if(fdd[j] < 0) { perror("error in opening device file\n"); exit(1);}
		ptr_fd[j] = &fdd[j];
	}
	
		printf("Enter 1 for write 2 for read\n");
		scanf("%d", &choice);		
	for(i=1; i<= no_device; i++)
	{
		switch(choice)
		{
			case 1:
					pthread_create(&twrite[i],NULL,fun_write,ptr_fd[i]);
					break;
			case 2:
					pthread_create(&tread[i],NULL,fun_read,ptr_fd[i]);
					break;
			default:
					printf("Sorry wrong entry \n"); exit(1);
		}
	}
	for(i=1; i<= no_device; i++)
	{
		switch(choice)
		{
			case 1:
					pthread_join(twrite[i],NULL);
					break;
			case 2:
					pthread_join(tread[i],NULL);
					break;
			default:
					break;
		}

	}
	for(i=1; i<= no_device; i++)
	{
		close(fdd[i]);
	}
	
	return 0;
}

void* fun_read(void* pv)
{
	char *buf, concat;
	int ret,ret1,fd, *fd1;
	fd1 =(int*)pv;
	
	printf("fd1 = %d\n",*fd1);
	buf = (char*)malloc(BUF_SIZE*sizeof(char));	
	ret1 = read(*fd1, buf, BUF_SIZE*sizeof(char));		
	printf("size of buf = %d info is %s\n", ret1, buf);
	if(ret1 < 0) { perror("error in reading pdinfo\n"); exit(1);}
	
	fd = open("read.txt", O_RDWR|O_CREAT,S_IRGRP);  //S_IRGRP	00040 group has read permission
	if(fd < 0) { perror("error in creating\n"); exit(1);}  /*mode must be specified when O_CREAT is in the 														flags, and is ignored otherwise */
	ret = write(fd, buf, ret1);
	if(ret < 0) { perror("error in reading\n"); exit(1);}

	
	close(fd);

	pthread_exit(0);
}

void* fun_write(void* pv)
{
	char *buf_write, info[] = "Hi this is writing to device file";	
	int ret,fd, sz, *fd1;
	fd1 =(int*)pv;
	printf("fd = %d\n",*fd1);
	sz = strlen(info);
	buf_write = (char*)malloc(BUF_SIZE*sizeof(char));
	strcpy(buf_write, info);

	ret = write(*fd1, buf_write, sz);
	if(ret < 0) { perror("error in writing\n"); exit(1);}
	printf("bytes written  = %d, buf_write = %s \n", ret, buf_write);

	
	pthread_exit(0);
}




