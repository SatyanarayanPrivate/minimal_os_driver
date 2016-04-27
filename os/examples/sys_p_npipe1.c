#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>


int main(int argc, char *argv[])
{

   char buf[512],wr_buf[512];	
   int ret,ret1,status;
   int npfd1,fd1,fd2,fd3,pfd[2];
   struct stat s1,s2;

   npfd1 = open(argv[1], O_WRONLY); //opening the named pipe for reading
                                    //only
   if(npfd1<0) {perror("error in opening the named pipe"); exit(1); }
	   
   strcpy(wr_buf, "data from the write process of the named pipe\n"); 

   ret1 = write(npfd1,wr_buf,strlen(wr_buf)+1);
 
   if(ret1<0){ perror("error in writing");exit(2);} 
	   
   close(npfd1);

   exit(0);

} 	 
	
