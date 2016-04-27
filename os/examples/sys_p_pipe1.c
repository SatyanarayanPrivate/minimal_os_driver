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
   int fd1,fd2,fd3,pfd[2];

   //after the pipe is created, the r/w handles are passed
   //in pfd[0] and pfd[1], respectively
   ret = pipe(pfd);

   if(ret<0) {perror("error in pipe"); exit(2); }

   ret = fork();//the parent has the handles to the pipe
                //the child also is given the handles

   if(ret<0) { };

   if(ret==0){
           //we are opening a regular file - passed as command-line arg.

	   //fd1 = open(argv[1],O_WRONLY);
	   //fd1 = open(argv[1],O_RDONLY);

	   //fd1 = open(argv[1],O_RDWR);
           //open may be used to create a file and open it or
           //just open a file that already exists
	   //fd1 = open(argv[1],O_CREAT|O_WRONLY|O_TRUNC,0600);
	   //fd1 = open(argv[1],O_CREAT|O_WRONLY|O_TRUNC,0600);
           //dup2(fd1,STDOUT_FILENO);
           //
           //or
           //close(STDOUT_FILENO); 
           //dup() system call copies the file-descriptor table entry
           //of the given index into the file-descriptor table entry
           //that is free and has the lowest index no.
           //dup(fd1);   //together, the 2 sys calls have completed the
                       //redirection of the standard o/p file-desc.
                       //of this process
	   
          /* while( (ret1=read(fd1,buf1,sizeof(buf1)) >0)
           {
                 ret2=write(pfd[1],buf1,ret1);
           }*/
           //close(fd1);    //this is optional - but, conventionally done
	   close(pfd[1]); //in this process, we are reading from the pipe
                          //close the write handle

           //in unix/linux, when you read from a file, the ret value 
           //of the system call will be non-zero, if data continues to
           //exist - on the other hand, ret value will be zero, if no
           //further data exists in the file or any other file-like 
           //object - meaning, end of file status
           //this is known as end-of-file in Unix/Linux language
           //the ret value is negative, if there is error 

           //when does end of file occur in this case ?
           //ret1 == 0 is the condition used to check it  
           //when does this condition actually occur ??
           //
           //in the case of pipe, when there is no data to be 
           //read, but one or more processes have the write handle
           //to the pipe, the read() system call will block
           //until there is more data 
           //
           //in the case of pipe, when there is no data to be read,
           //but no process has a write handle to the pipe, 
           //the read() system call will not block - it will return
           //0 - meaning, end of file is notified 

           //what happens, if a process is writing to a pipe and
           //one or more processes have the read handle to the pipe,
           //open and there is no space in the pipe buffer ?? when
           //there is more space, the process will be woken-up 

           //what happens, if a process is writing to a pipe and
           //no process has the read handle to the pipe, open and
           //there is no more space in the pipe buffer ?? 
         
           //what happens, if a process is writing to a pipe and
           //no process has the read handle to the pipe, open and
           //there is ample  space in the pipe buffer ??

           //in the last 2 cases, the write is not allowed to proceed
           //and the process that is attempting to write will sent
           //a SIGPIPE signal by the system 
   

	   while( (ret1 = read(pfd[0],buf,512)) >0)
	   {
		   //printf("%s\n", buf);
                   write(STDOUT_FILENO,buf,ret1);
		   //fflush(stdout);
	   }  
	   if(ret1<0){ } 
	   close(pfd[0]);
   }

   if(ret>0)
   {
      close(pfd[0]);   //we are interested in writing only
      write(pfd[1],"1..this is a message from parent",33);//no formatting 
      write(pfd[1],"2..this is a message from parent",33); 
      //write(pfd[1],wr_buf,sizeof(wr_buf)); //this is wrong
      //write(pfd[1],wr_buf,strlen(wr_buf)+1); //this is right 
      close(pfd[1]);
   }
   
   exit(0);

} 	 








	
