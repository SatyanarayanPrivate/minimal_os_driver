#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<sched.h>


int main()
{
	int i=0,ret,status,ret1;
	//printf("the value of ret is %d\n\n",ret);

	
	
	
	ret=fork();
//	pause();
	
	printf("the value after fork is %d\n\n ",ret);
	if(ret<0)
	{
		perror("\n An error occured in fork..\n");
		exit(1);
	}
	if(ret>0)
	{
	printf("its in parent\n");
	ret=waitpid(-1,&status,0);
	
		if(ret>0)
		{
			if(WEXITSTATUS(status))
			{
				if(WIFEXITED(status)==0)
				{
					printf("cleaned up");
				}
			      else
				{
					printf("\n normal but didnt succeed\n");
		    
				} 
			}
		}
	
    	else 
	    {
		  exit(1);
		    
	    }
	}

	if(ret==0)
	{ 
	        ret1=sched_yield();
		if(ret1==0)
		{
			printf("success\n");
		}
		else
			
		{
			printf("Failed\n");		
		}
	
	
		printf("the childs pid is %d,,,,,,,,,,and ppid is %d\n",getpid(),getppid());

	}



}

