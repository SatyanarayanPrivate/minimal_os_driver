
#include<stdlib.h>
#include<string.h>

int main()
{

  sigset_t s1,s2;

  char buf1[512];

  struct sigaction act1,act2;

  int ret;

  sigemptyset(&s1);
  sigaddset(&s1,SIGINT);
  sigaddset(&s1,SIGQUIT);

  act1.sa_handler = SIG_DFL; //default action to be installed

  sigfillset(&act1.sa_mask); // all signals are masked during 
                             // action
  act1.sa_flags = 0;         // no flags

  sigaction(SIGINT,&act1,NULL);  // installing the action
                                 // for SIGINT

  act1.sa_handler = SIG_DFL; //default action to be installed

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
