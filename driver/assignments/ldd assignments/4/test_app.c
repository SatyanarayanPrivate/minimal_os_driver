#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MAX_BUF 1024

void *read_dev (void *arg);	// READ
void *write_dev (void *arg);	// WRITE

char *write_buf[] = {
			"Hello Device 0",
       			"Hello Device 1", 
			"Hello Device 2",
	       		"Hello Device 3",
	       		"Hello Device 4" };

int main ( int argc, char *argv[] ) //./test /dev/p0 /dev/p1 /dev/p2 /dev/p3 /dev/p4
{
	// int pthread_create (pthread_t *thread, pthread_attr_t *attr, void* (*start_routine)(void), void *arg);	
	// void pthread_exit (void *retval);	-- Terminates calling thread
	// int pthread_join (pthread_t th, void **thread_return); -- join with a terminated thread

	int ret, i;
	pthread_t rthr[5], wthr[5];	
	int dfd;	// Device file descriptor
	for (i=0; i<5; i++)
	{
		// int open(const char *pathname, int flags);
		dfd = open (argv[i+1], O_RDWR);	// Return file descriptor
	
		if (dfd == -1 )
		{
			printf ("Error in opening device\n");	
			exit (0); //must terminate with error 
		}
		
		// int mknod(const char *pathname, mode_t mode, dev_t dev);	-- create a special or ordinary file
			
		ret = pthread_create (&wthr[i], NULL, write_dev, (void *)&dfd);	// Write call
		if (ret != 0)
		{
			perror ("Thread creation failed\n");
			exit (EXIT_FAILURE);
		}

		ret = pthread_create (&rthr[i], NULL, read_dev, (void *)&dfd);	// Read call
		if (ret != 0)
		{
			perror ("Thread creation failed\n");
			exit (EXIT_FAILURE);
		}		
	}

	for (i=0; i<5; i++)
	{
		ret = pthread_join (wthr[i], NULL);
		if (ret != 0)
			printf ("Error in joining write thread\n");

		ret = pthread_join (rthr[i], NULL);
		if (ret != 0)
			printf ("Error in joining write thread\n");
	}
	return 0;
}

//--------------------------------------------------------------------------------------------------------------------
void *read_dev (void *arg)	// READ
{
	int bytes;
	char read_buf[MAX_BUF];
	
	// ssize_t read(int fd, void *buf, size_t count); -- Read from file descriptor
	bytes = read (*(int *)arg, read_buf, MAX_BUF);
	if (bytes == -1)
		printf ("Error in reading device\n");
	else
		printf ("Number of bytes read = %d\n", bytes);

	printf (" ** Message received: %s\n", read_buf);	
	pthread_exit ("Read thread function finished\n");
}

void *write_dev (void *arg)	// WRITE
{
	int bytes;
	static int j=0;

	// size_t write(int fd, const void *buf, size_t count); -- Write to a file descriptor
	bytes = write (*(int*)arg, write_buf[j++], strlen(write_buf[j])+1 );
	if (bytes == -1)
		printf ("Error in writing device\n");
	else
		printf ("Number of bytes written = %d\n", bytes);
	
	pthread_exit ("Write thread function finished\n");
}

//--------------------------------------------------------------------------------------------------------------------
