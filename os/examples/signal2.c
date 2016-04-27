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


int main()
{

  sigset_t s1,s2;

  char buf1[512];

  struct sigaction act1,act2;

  int ret;

  sigemptyset(&s1);
  sigaddset(&s1,SIGINT);
  sigaddset(&s1,SIGQUIT);

  //act1.sa_handler = SIG_IGN; //default action to be installed
  act1.sa_handler = sig_handler1;
  sigfillset(&act1.sa_mask); // all signals are masked during 
                             // action
  act1.sa_flags = 0;         // no flags

  sigaction(SIGINT,&act1,NULL);  // installing the action
                                 // for SIGINT

  //act1.sa_handler = SIG_IGN; //default action to be installed
  act1.sa_handler = sig_handler2;

  sigfillset(&act1.sa_mask); // all signals are masked during 
                             // action
  act1.sa_flags = 0;         // no flags

  sigaction(SIGQUIT,&act1,NULL);  // installing the action
                                 // for SIGQUIT

  //sigfillset(&s1);

  //sigprocmask(SIG_BLOCK,&s1,&s2);
  //sigprocmask(SIG_UNBLOCK,&s1,&s2);
  //sigprocmask(SIG_SETMASK,&s1,&s2);

  //ret = fork();

  //if(ret==0) setpgid(0,getpid());

  printf("hello world !! in process with pid %d and pgid  %d\n",getpid(),getpgid(0));


  
  while(1);

}
