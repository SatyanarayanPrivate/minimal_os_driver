#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,status,ret1;

   unsigned long int i=0;

  while(i++<5)
  {

   ret = fork();
   printf("the value is %d \n",ret);
 
   if(ret<0)
   { 
           perror("error in fork"); 
           printf("the final value of i is %lu\n", i);
           exit(1); 
   }


   if(ret>0)
   { 
	   printf("\nI am in parent process context\n"); 
           printf("in parent .. ppid is %lu ...and pid is %lu\n\n\n", getppid(),getpid());	   
	    continue;
   }


   if(ret==0) 
   { 
	   printf("\nI am in child process context\n"); 
           printf("in child .. ppid is %lu ...and pid is %lu\n\n", getppid(),getpid());	   
	    exit(0);
 
   }

 } 

printf("\n the pid bfor waitpid is %d\n",ret);
 if(ret>0)
 {
   while(1)
   {  
    ret1 = waitpid(-1,&status,0);
    if(ret1>0)
    {

   	 if(WIFEXITED(status))    
   	 {
       	if(WEXITSTATUS(status) == 0)
	   {
		   printf("Cleanupin progress for %d\n",ret1); 
		   
	   }
       else{
	       printf("normal termintation but not successful\n");

       	   }
   	 }
       else
    	{ 
		printf("abnormal termination\n");
    	}
   }
    if(ret1<0)
    {
	    exit(1); 
    } 
   }  
 } 

return 0;
   
}


