#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,status,parentid;

   char *argv1[] = { "vi", "fork_yield.c", NULL }; 

   parentid = getpid();

   ret = fork();
 
   if(ret<0){ perror("error in fork"); exit(1); }

   //if(getpid() == parentid){

   if(ret>0){ 
          //any useful code of the parent process can exist here
          printf("in parent context pid is %d and ppid is %d\n", 
                                    getpid(), getppid()); 

	   
	   ret = waitpid(-1,&status,0); 
           if(ret>0) {
                   printf("child with pid %d cleaned-up\n", ret); 
                   if(WIFEXITED(status))
                     printf("normal termination and the exit status is %d\n", WEXITSTATUS(status));
                   else
                     printf("the process terminated abnormally\n");  
           }
           pause();
           //sleep(10);   
	   exit(0); 
   }

  else { 
          printf("in child context pid is %d and ppid is %d\n", 
                                    getpid(), getppid()); 
	   
   //pause();    
           //
   ret = execv("/usr/bin/vi", argv1);  
   //ret = execlp("vi", "vi", "fork_yield.c", NULL);  
   //ret = execl("/usr/bin/vi", "vi", "fork_yield.c", NULL);  
   //ret = execl("/usr/bin/vi", "vi", "fork_yield.c", NULL);  
   //ret = execl("/usr/bin/gcc","gcc","fork1n.c", "-o", "fork1n",NULL);  
   //ret = execl("/usr/bin/gcc","gcc","-c", "fork1n.c", "-o", "fork1n.o",NULL);  
   if(ret<0) { perror("error in execl"); exit(33); }      

   pause();    
   exit(0); 
  }

return 0;
   
}


