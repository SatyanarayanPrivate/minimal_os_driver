#include<fcntl.h>

int main()
{

   char buf[512];	
   int ret,ret1,status;
   int fd1,fd2,fd3;

   fd1 = open("/etc/passwd", O_RDONLY);
   if(fd1<0){ perror("error in open1"); exit(1);} 

   fd3 = open("/etc/passwd", O_WRONLY);
   if(fd1<0){ perror("error in open1"); exit(1);}

  //O_SYNC is a flag that enables all writes associated with 
  //this active file to be updated synchronously  and the 
  //process blocks for the writes to complete !!!
  //
   fd2 = open("/home/corporate/passwd.bk", O_RDWR|O_CREAT|O_SYNC, 0640);
   if(fd2<0){ perror("error in open2"); exit(2);}

   while(1){
  	 ret = read(fd1,&buf,sizeof(buf));   
	 if(ret<0) { perror("error in read"); exit(3);}
	 if(ret == 0) break; //the read system call returns zero
                             //when the end of the file is reached
                             //and there is no more data in the 
                             //file 
	 ret1 = write(fd2,buf,ret);
	 if(ret1<0){ perror("error in write"); exit(4);}
   }	 
   close(fd1);
   close(fd2); 

} 	 
	
