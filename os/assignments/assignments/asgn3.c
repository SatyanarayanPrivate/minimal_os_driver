#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,status;//parentid;

   //parentid = getpid();

   ret = fork();

   if(ret<0){ perror("error in fork"); exit(1); }


   if(ret>0)
   { 
          printf("in parent context pid is %d and ppid is %d\n",getpid(), getppid()); 
	  ret = waitpid(-1,&status,0); 
           if(ret>0)
	   {
                   printf("child with pid %d cleaned-up\n", ret); 
                   if(WIFEXITED(status))
                     printf("normal termination and the exit status is %d\n", WEXITSTATUS(status));
                   else
                       printf("the process terminated abnormally\n");  

          }

	   exit(0); 
   }

  else { 
           printf("in child context pid is %d and ppid is %d\n",getpid(), getppid()); 
	   ret = execl("/usr/bin/vi", "vi", "fork2n.c", NULL);  
       }
  

   if(ret<0)
   { 
	   perror("error in execl"); 
	   exit(33);
   }     
  
   printf("my program is successfully loaded \n"); 
    
  

return 0;
   
}


