#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{
	int i=0,ret,status;
	printf("the value of ret is %d\n\n",ret);
	ret=fork();
	printf("the value after fork is %d\n\n ",ret);
	
	if(ret>0)
	{
	printf("its in parent\n");
	}
	if(ret==0)
	{
		printf("the child's ppid is %d,,,,,,,,the pid is %d....\n\n",getppid(),getpid());
		exit(0);
	}

	printf("\n the value of ret befor waitpid  is  %d.....\n\n",ret);
	if(ret>0)
	{
		while(1)
		{
			ret=waitpid(-1,&status,0);
			printf("the value of ret after waitpid is %d.....\n",ret);
			if(ret>0)
			{
				if(WIFEXITED(status))
				{
				if(WEXITSTATUS(status) == 0)
				{
					printf("\n normal teminationand success\n");
				}
				
				else{
					printf("\n normal but not successful\n");
				}
				}
				else printf("\n abnormal but not suucccesss\n");
			}
			if(ret<0)
				exit(0); 	

				
			}
		} 
	}







