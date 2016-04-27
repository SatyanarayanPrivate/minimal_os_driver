#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,status;

   unsigned long int i=0;

  while(i++<5){

   ret = fork();
// printf("the value is: %d\n\n",ret);
   if(ret<0){ 
           perror("error in fork"); 
           printf("the final value of i is %lu\n", i);
          
           exit(1); 
   }

   if(ret>0){ 
	   printf("I am in parent process context\n"); 
           printf("in parent .. ppid is %lu ...and pid is %lu\n\n", 
		   getppid(),getpid());	   
	   
           exit(0);
   }

   if(ret==0) { 
	   printf("I am in child process context\n"); 
           printf("in child .. ppid is %lu ...and pid is %lu\n\n", 
		   getppid(),getpid());	   
	   

	   
           continue;
   }

 }//while


 if(ret>0)
 {
   while(1){ 
    ret = waitpid(-1,&status,0);
    if(ret>0){

    if(WIFEXITED(status)) 
    {
       if(WEXITSTATUS(status) == 0){ 
       }
       else{

       }
    }
    else{ 
    }
   }//ret>0

    if(ret<0) { exit(0); }
   } //second while 
  }//if after while loop 

return 0;
   
}


