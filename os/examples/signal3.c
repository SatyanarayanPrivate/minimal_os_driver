#include<stdio.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>


void sig_handler1(int signo)
{

  write(STDOUT_FILENO,"received SIGINT in handler",25);
  

} 

void sig_handler2(int signo)
{

  write(STDOUT_FILENO,"received SIGQUIT in handler",25);
  

}

void sig_handler3(int signo)
{

  write(STDOUT_FILENO,"received SIGCHLD in handler",25);
  

}

int main()
{

  sigset_t s1,s2,s3;

  char buf1[512];

  struct sigaction act1,act2;

  int ret,i=1;

  sigfillset(&s1);
  
  sigprocmask(SIG_SETMASK,&s1,&s2);  
  //act1.sa_handler = SIG_IGN; //default action to be installed
  act1.sa_handler = sig_handler1;
  sigfillset(&act1.sa_mask); // all signals are masked during 
                             // action
  act1.sa_flags = 0;         // no flags

  sigaction(SIGINT,&act1,NULL);  // installing the action
                                 // for SIGINT
  
  act1.sa_handler = sig_handler3;
  sigfillset(&act1.sa_mask); // all signals are masked during 
                             // action
  act1.sa_flags = 0;         // no flags

  sigaction(SIGCHLD,&act1,NULL);  // installing the action
                                 // for SIGINT
  while(i<=5)
  {
    ret = fork();

    if(ret<0) { //error 
        }

      if(ret>0)
	{

	  i++;
          printf("parent in iteration %lu\n", i);

	}

      if(ret==0){
	printf("I am in child %lu\n", i);
          sleep(i);
          exit(i);
      }

    }

    sigfillset(&s3);
    sigdelset(&s3,SIGCHLD);
    sigdelset(&s3,SIGINT);

    while(1)
      {

        ret = sigsuspend(&s3);
        if(ret<0) printf("return from sigsuspend is %lu\n", ret);
      }
    

    exit(0);

}
