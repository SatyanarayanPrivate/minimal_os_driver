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
   struct stat s1,s2;

   if(argc < 2) { printf("enter file name\n"); exit(1);}
   //after the pipe is created, the r/w handles are passed
   //in pfd[0] and pfd[1], respectively
   ret = pipe(pfd);

   if(ret<0) {perror("error in pipe"); exit(2); }

 while(i<2)
 {

   ret = fork();//the parent has the handles to the pipe
                //the child also is given the handles

  

   if(ret<0) { };

   if(ret==0 && i==0){
           //we are opening a regular file - passed as command-line arg.

	   fd1 = open(argv[1],O_WRONLY);
	   //fd1 = open(argv[1],O_RDONLY);

	   //fd1 = open(argv[1],O_RDWR);
           //open may be used to create a file and open it or
           //just open a file that already exists
	   //fd1 = open(argv[1],O_CREAT|O_WRONLY|O_TRUNC,0600);
	   //fd1 = open(argv[1],O_CREAT|O_WRONLY|O_TRUNC,0600);
           //dup2(fd1,STDOUT_FILENO);
           //
           //or
           //here we are releasing or freeing the entry corresponding
           //to index 1, in the open file descriptor table
           close(STDOUT_FILENO); 
           //dup() system call copies the file-descriptor table entry
           //of the given index into the file-descriptor table entry
           //that is free and has the lowest index no.
           dup(fd1);   //together, the 2 sys calls have completed the
                       //redirection of the standard o/p file-desc.
                       //of this process to now point to the 
                       //open file of the regular file, just opened
	   
          /* while( (ret1=read(fd1,buf1,sizeof(buf1)) >0)
           {
                 ret2=write(pfd[1],buf1,ret1);
           }*/
           close(fd1);    //this is optional - but, conventionally done
	   close(pfd[1]); //in this process, we are reading from the pipe
                          //close the write handle

           //in unix/linux, when you read from a file, the ret value 
           //of the system call will be non-zero, if data continues to
           //exist - on the other hand, ret value will be zero, if no
           //further data exists in the file or any other file-like 
           //object - meaning, end of file status
           //this is known as end-of-file in Unix/Linux language
           //the ret value is negative, if there is error 

	   while( (ret1 = read(pfd[0],buf,512)) >0)
	   {
		   //printf("%s\n", buf);
                   write(STDOUT_FILENO,buf,ret1);
		   //fflush(stdout);
	   }  
	   if(ret1<0){ } 
	   close(pfd[0]);
   }

 //while loop
  

   if(ret==0 && i==1)
   {
      close(pfd[0]);   //we are interested in writing only
      write(pfd[1],"1..this is a message from parent",66);//no formatting 
      write(pfd[1],"2..this is a message from parent",33); 
      write(pfd[1],wr_buf,sizeof(wr_buf)); //this is wrong
      write(pfd[1],wr_buf,strlen(wr_buf)+1); //this is right 
      close(pfd[1]);
   }

  } //while loop 
  
   close(pfd[0]);
   close(pfd[1]); 

   if(ret>0){
       while(1){
          ret1 = waitpid(-1,&status,0);
          if(ret1<0) break;
          if(WIFEXITED(status) && (WEXITSTATUS(status)!=0))
          {

             printf("a child terminated unsuccessfully\n");
             child_unsuccessfull++;
          }  


       }
   }

 
   exit(0);

} 	 
	
