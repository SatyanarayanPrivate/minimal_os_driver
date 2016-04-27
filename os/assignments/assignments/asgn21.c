#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,status;
   unsigned long i=0, j, cleanupcounter=0;

while(1)
{	
   ret=fork();
 
   if(ret<0)
   { 
     perror(NULL); 
     break; 
   }

   if(ret>0)
   { 
	i++;
	printf("\n I am in parent process context \n");
	continue;
	}
  
  if(ret==0) 
   { 	 
	   printf("the child process created are %lu\n",i);
           printf("in child .. ppid is %d ...and pid is %d\n\n", getppid(),getpid());	   
	exit(0);
   }	
}	
   
//pause();

 if(ret<0)
{
   printf("in ret11  %d...........%d\n",ret,i);
   while(1)
   {
	ret = waitpid(-1,&status,0);
           //cleanupcounter++;
   	printf("in ret %d....\n",ret);
   	if (ret>0)
	{
		cleanupcounter++;
		printf("in retff \n");
    		if(WIFEXITED(status))     //normal termination of the process
    		{
		    if(WEXITSTATUS(status) == 0)
		    {
			    printf("\n pid %d cleaned up\n",ret);//normal and successfull
    			   // cleanupcounter++;   //once again, you have got the logic wrong !!!
      		    }
		    else
		    {
			    printf("\n normal but didnt succeed\n");
		    
		    } 
       		}
	}
    	else 
	    {
		 printf("\n the last child terminated");
		    break;
		    
	    }
     }

   }
printf(" No. of process created  %lu  No. of process cleaned up  %lu \n",i,cleanupcounter);
}
