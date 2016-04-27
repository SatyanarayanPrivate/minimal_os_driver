#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>

sigset_t s1,s2;

struct sigaction act1,act2;

void sg_in(int signo)
{
	printf("sig int handler\n");
         kill(getpid(),SIGQUIT);	
     
}



void sg_term(int signo)
{
   
	       	printf("sig term, handler\n");
 	 	 //kill(getpid(),SIGQUIT);		
}


void sg_quit(int signo)
{
		printf("sig quitt handler\n");
  		// kill(getpid(),SIGKILL);	
     

	}


void sg_tstp(int signo)
{		printf("sig tstp handler\n");
  		// kill(getpid(),SIGQUIT);	
     

		
}


void sg_stop(int signo)
{			printf("sig stop handler");
  			// kill(getpid(),SIGQUIT);	
     
}
	

void sg_kill(int signo)
{
			printf("sig kill handler");
   			//kill(getpid(),SIGKILL);	
     
	    

}






int main()
{
	int ret;
	sigset_t set1,set2;
	struct sigaction act1,act2;


	 
       //	sigprocmask(SIG_SETMASK,&set1,&set2);
	 sigfillset(&set1);
	
       	sigprocmask(SIG_SETMASK,&set1,&set2);
//	 sigfillset(&set1);
	 sigdelset(&set1,SIGINT);
	 sigdelset(&set1,SIGTERM);
	 sigdelset(&set1,SIGQUIT);
	 sigdelset(&set1,SIGTSTP);
	 sigdelset(&set1,SIGSTOP);
	 sigdelset(&set1,SIGKILL);
	 
	 
       //	sigprocmask(SIG_SETMASK,&set1,&set2);


        act1.sa_handler = sg_in;
        act1.sa_flags = 0; 
        sigfillset(&act1.sa_mask);
        sigaction(SIGINT,&act1,&act2);

	
       
	act1.sa_handler = sg_term;
	 act1.sa_flags = 0;
	 sigfillset(&act1.sa_mask);
	 sigaction(SIGTERM,&act1,&act2);



	 act1.sa_handler = sg_quit;
         act1.sa_flags = 0;
	 sigfillset(&act1.sa_mask);
	 sigaction(SIGQUIT,&act1,&act2);



	 act1.sa_handler = sg_tstp;
         act1.sa_flags = 0;
	 sigfillset(&act1.sa_mask);
	 sigaction(SIGTSTP,&act1,&act2);
	 
	 act1.sa_handler = sg_stop;
         act1.sa_flags = 0;
	 sigfillset(&act1.sa_mask);
	 sigaction(SIGSTOP,&act1,&act2);
	 
	 act1.sa_handler = sg_kill;
         act1.sa_flags = 0;
	 sigfillset(&act1.sa_mask);
	 sigaction(SIGKILL,&act1,&act2);
         
	 //here block using a single sigsuspend() and send appropriate signals from 
	 //command line using kill command !!!
	 printf("before sigsuspend\n");
	 ret=sigsuspend(&set1);
	 printf("after sigsuspend\n");



return 0;
}
	
