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

   //mkfifo() creates a special file known as pipe file
   //this file does not store any data blocks - it just 
   //has an inode and directory entry 
   //after creating this file, developer can use it with
   //open(), read(), write() , close() and other relevant
   //file system call APIs

   //such a pipe file can also be created using mkfifo utility !!!
   //such a special file is used along with the help of 
   //logical file system manager and pipe IPC component - 
   //there may not be a file system maanager in this context !!! 


   ret = mkfifo(argv[1],0600);
   //ret is just the return value used for error checking 
 
  //after this open() system call API on a name pipe file, 
  //a setup of the pipe object, pipe buffer and an active
  //file are created as shown the class diagram 

  //open() must be called with O_RDONLY or O_WRONLY - 
  //in doing so, after the above setup is created, 
  //current process is blocked in the wq of the 
  //pipe object until another process opens the pipe file
  //for read or write , accordingly 

  //it is a natural expectation that a process using a pipe 
  //for IPC will open it for reading and another process will
  //open it for writing or vice-versa - however, pipe IPC 
  //mechanism is uni directional and one process can read and 
  //the other can write - not bidirectional 

  //you can always setup multiple pipe IPc mechanisms to 
  //implement bidirectional communication 

  npfd1 = open(argv[1], O_RDONLY); //opening the named pipe for reading
   printf("this is after open \n"); 
                                    //only
   if(npfd1<0) {perror("error in opening the named pipe"); exit(1); }
	   

  //assuming that a communication channel has been established 
  //between 2 processes via this pipe file, read() system call
  //will block until there is data 

  //if there is some data in the pipe buffer, read will return 
  //after copying that data - the ret value signifies amount of 
  //data copied 

  //if the current process is blocked in the read(), data
  //is written into it from the other process, it is the responsibility
  //of the writing process(write() ) to wake up the read process

  //if there is no data in the pipe buffer and there is a write process
  //with a write handle to this pipe alive, end of file will never
  //be seen by the read process !!!

  //if the write process closes the write handle to this pipe or
   //the write process terminates, the read process will see an
  //end of file - this is a subtle difference with respect to 
  //regular files 

  //unlike in the case of regular files, random access of pipe
  //buffer is not allowed - meaning, lseek() or similar system call
  //APIs will return error !!!

  //the following comments apply to a process that writes to 
  //the pipe :

  //if there is no space in the pipe buffer and a process attempts
  //to write to the pipe, current process will be blocked

  //it is the responsibility of the read process (read()) to wakeup
  //the write process, when read is done 

  //if the read process closes the read handle to the pipe or read 
  //process terminates, any attempt to write by the process will
  //lead to generation of SIGPIPE signal from the system to 
  //the write process - the consequence of this SIGPIPE is termination
  //of the write process - such run time issues are very common if
  //processes do not understand the exact working of pipes - 
  //it is the responsibility of the developer to install a handler
  //for such situations and capture such bugs - subsequently, 
  //such errors should be rare during run time - meaning, such 
  //errors will occur only if the read process terminates abnormally !!!

  //such a handler may not do great run time activity - 
  //its main job will be free resource, shutdown devices and
  //terminate with a diagnostic message - this is to help
  //the developer and the system !!!



  








   while( (ret1 = read(npfd1,buf,512)) >0)
	   {
		   //printf("%s\n", buf);
                   write(STDOUT_FILENO,buf,ret1);
		   //fflush(stdout);
	   }  
	   if(ret1<0){ } 
	   close(npfd1);

   exit(0);

} 	 
	
