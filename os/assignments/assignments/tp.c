#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,status;
   unsigned long i=0, j, cleanupcounter=0;
   
   

while(++i)
{	
   ret=fork();
      
 
   if(ret<0)
   { 
     perror("error in fork\n"); 
     break; 
   }

   if(ret>0)
   { 
	printf("\n I am in parent process context \n");
	continue;

   }

	  if(ret==0) 
   { 
	   printf("the child process created are %l\n",i);
           printf("in child .. ppid is %d ...and pid is %d\n\n", getppid(),getpid());	   
  	   exit(0);
		 
	 }	
}	
   
    
 if(ret<0)
{
  printf("\n %d is the no of pro",i);
   j=i;
   printf("in ret11  %d\n",ret);
   ret = waitpid(-1,&status,0);
   if(ret>0)
   {
   printf("in ret %d....\n",ret);
   while(1)
     {
	printf("in retff \n");
    	if(WIFEXITED(status))     //normal termination of the process
    	{
	    if(WEXITSTATUS(status) == 0)
	    {
		    printf("\n cleanup\n");//normal and successfull
    		    cleanupcounter++;
		    if(ret<0)
		    {
			    printf("\n the process are %d",cleanupcounter);
			    exit(1);
		    }
      	    }
	    else
	    {
		    printf("\n normal but didnt succeed\n");
		    exit(1);
		    
		    
            } 
       	}
	    else
	    {
		    printf("abnormal\n");
		    exit(1);
		    
	    }
	    if(WEXITSTATUS(status)<0)
	    {
		    printf("\n the process cleaned are  %d ",cleanupcounter);
		    exit(1);
	    }
     }

   }}
printf(" No. of process created  %lu  No. of process cleaned up  %lu \n",j,cleanupcounter);
}
