#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <signal.h> 

sigset_t s1,s2;

struct sigaction act1,act2;


void sig_hdl1(int signo)  //this handler is used to setup custom
                          //action for SIGTERM 
{
  printf("a dummy signal handler1  SIGINT\n");
  kill(getpid(),SIGQUIT); 

}  
void sig_hdl2(int signo) //this handler is used to setup custom 
                         //action for SIGINT 
{
  printf("a dummy signal handler2  SIGQUIT\n");
 kill(getpid(),SIGKILL); 
 printf("in 2\n");
}
void sig_hdl3(int signo)  //this handler is used to setup custom
                          //action for SIGTERM 
{
  printf("a dummy signal handler3 SIGTERM\n");
 kill(getpid(),SIGKILL); 
  printf("3\n");

}  
void sig_hdl4(int signo) //this handler is used to setup custom 
                         //action for SIGINT 
{
  printf("a dummy signal handler4  SIGKILL\n");
 //kill(getpid(),SIGKILL); 
printf("in 4\n");
}


void sig_hdl5(int signo) //this handler is used to setup custom 
                         //action for SIGINT 
{
  printf("a dummy signal handler5  SIGSTOP\n");
 //kill(getpid(),SIGKILL); 
printf("in 5\n");
}

void sig_hdl6(int signo) //this handler is used to setup custom 
                         //action for SIGINT 
{
  printf("a dummy signal handler6  SIGTSTP\n");
 kill(getpid(),SIGTERM); 
printf("in 6\n");
}








int main(int argc, char *argv[]) 
 { 

     int ret;
     sigset_t set1,set2;

     

   //  if (argc != 2) 
     //    exit(0); 
 
     sigfillset(&set1);  //every bit in this bit map is set !!!
     sigprocmask(SIG_SETMASK, &set1,&set2) ;
       printf("we are blocked in the first sigsuspend\n"); 
     sigdelset(&set1, SIGQUIT);
     sigdelset(&set1, SIGTERM); 
     sigdelset(&set1, SIGINT); 
     sigdelset(&set1, SIGSTOP); 
     sigdelset(&set1, SIGTSTP); 
     sigdelset(&set1, SIGKILL);
     

 
  
printf("step1\n");

     
     act1.sa_handler = sig_hdl1;  //ptr to our signal handler, in user space !!!
     act1.sa_flags = 0;  //currently, we ignore the flags
    sigfillset(&act1.sa_mask);
    sigaction(SIGINT,&act1,&act2); 
   
   

   
     act1.sa_handler = sig_hdl2;
     act1.sa_flags = 0;  //currently, we ignore the flags
     sigfillset(&act1.sa_mask);
     sigaction(SIGQUIT,&act1,&act2);
     
     

     act1.sa_handler = sig_hdl3;  //ptr to our signal handler, in user space !!!
     act1.sa_flags = 0;  //currently, we ignore the flags
    sigfillset(&act1.sa_mask);
    sigaction(SIGTERM,&act1,&act2); 
   
   

   
     act1.sa_handler = sig_hdl4;
     act1.sa_flags = 0;  //currently, we ignore the flags
     sigfillset(&act1.sa_mask);
     sigaction(SIGKILL,&act1,&act2);


     
      act1.sa_handler = sig_hdl5;  //ptr to our signal handler, in user space !!!
     act1.sa_flags = 0;  //currently, we ignore the flags
    sigfillset(&act1.sa_mask);
    sigaction(SIGSTOP,&act1,&act2); 
   
   

   
     act1.sa_handler = sig_hdl6;
     act1.sa_flags = 0;  //currently, we ignore the flags
     sigfillset(&act1.sa_mask);
     sigaction(SIGTSTP,&act1,&act2);

     
     
     
     
     
     printf("abov sigsuspend\n");
     ret = sigsuspend(&set1);//this process normally blocks here 
     printf("below sigsuspend\n");

     ret = sigsuspend(&set1);//this process normally blocks here 
     printf("below sigsuspend2\n");
     
     
     return 0; 




}
