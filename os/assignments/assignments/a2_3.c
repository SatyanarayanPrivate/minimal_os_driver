#include<sys/msg.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>
#include<errno.h>

int j=0;
void sig_chld(int signo)
{
int ret,status;
while(1)
{
ret=waitpid(-1,&status,WNOHANG);
	if(ret==0)
	{printf("\nstill child executing\n");
	break;
	}
	if(ret<0)
	break;
	if(ret>0)
	{
	j++;
		printf("\nchild cleaned up\n ");
	}

}
}
struct sigaction act1,act2;





int main()
{
	int i=0;
sigset_t s1,s2;
int ret,ret1;

	sigfillset(&s1);
	sigdelset(&s1,SIGCHLD)	;
	sigprocmask(SIG_SETMASK,&s1,&s2);
while(i++<5)
{
	 ret=fork();	
	if(ret<0)
	{
	perror("error in fork");
	exit(1);
	}
	if(ret>0)
	{
	printf("I am in parent process\n");
	continue;
	}
	if(ret==0)
	{
	printf("getppid()=%d      getpid()=%d",getppid(),getpid());
		if(i==2)
		{
		ret=execl("/usr/bin/gedit","gedit","abc.txt",NULL);
		exit(25);
		}
		else
	exit(0);
	}
}
	act1.sa_handler=sig_chld;
	sigfillset(&act1.sa_mask);
	sigaction(SIGCHLD,&act1,&act2);
	while(j<5)
        {
         ret1 = sigsuspend(&s1);
        } 
printf("\n now parent will terminate");
}
