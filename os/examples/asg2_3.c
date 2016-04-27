#include<stdio.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<errno.h>
#include<sys/wait.h>

int j = 0;
void
sig_child (int signo)
{
  int ret, status;
  // j++;
  while (1)
    {
      ret = waitpid (-1, &status, WNOHANG);
      if (ret < 0)
	{
	  printf ("no children\n");
	  break;
	  //exit(1);
	}

      if (ret > 0)
	{
	  printf ("in signal handler ....cleanupwith child id= %d \n", ret);
	  j++;
	  //continue;
	}
      if (ret == 0)
	{
	  printf ("no child in zombie state\n");
	  break;

	}
    }

}

int
main ()
{
  int r, status, i = 0, ret1;

  struct sigaction act1, act2;
  sigset_t set1, set2;
  sigfillset (&set1);
  sigdelset (&set1, SIGCHLD);
  sigprocmask (SIG_SETMASK, &set1, &set2);
  act1.sa_handler = sig_child;
  act1.sa_flags = 0;
  sigaction (SIGCHLD, &act1, &act2);





  while (i < 5)
    {
      r = fork ();

      if (r < 0)
	{
	  printf ("error");
	  exit (1);
	}
      if (r > 0)
	{
	  printf (" i m in parent context pid....%d     ppid...%d\n",
		  getpid (), getppid ());
	  i++;
	}
      if (r == 0)
	{
	  printf (" i m in child context pid....%d     ppid...%d\n",
		  getpid (), getppid ());
	  //      i++;
	  exit (0);
       }
     }

     if (r > 0)
	{
	  while (j <= 4)

	    {
	      ret1 = sigsuspend (&set1);
	      printf ("ret value after sigsuspend is=%d\n", ret1);
	    }

	}
    
  return 0;

}
