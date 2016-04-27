#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,status;

   unsigned long int i=0;

  while(i++<5){ //change this with a reasonable condition !!!

   ret = fork();
 
   if(ret<0){ 
           perror("error in fork"); 
           printf("the final value of i is %lu\n", i);
          
           //an example for normal termination, but not successful 
           exit(1); 
   }

   if(ret>0){ 
	   printf("I am in parent process context\n"); 
           printf("in parent .. ppid is %lu ...and pid is %lu\n", 
		   getppid(),getpid());	   
	   
           //++i;     //ok

           //read the comments below - if you understand the comments,
           //what happens, if we add waitpid() in this block of code,
           //not in the while loop below 
           //if we use waitpid() here, parent process will be blocked
           //until the recently created child process is terminated and
           //cleaned-up - which means, several children processes cannot 
           //co-exit actively - which means, multiprogramming/multitasking
           //is lost - in addition, try to visualize the same problem
           //in a multiprocessing (MP) environment - the efficiency 
           //will be simply unacceptable !!!

           //
           //using waitpid() is not efficient !!!
           //ret=waitpid() 

           //continue;
           //exit(0);
           break ; 
   }

   if(ret==0) { 
	   printf("I am in child process context\n"); 
           printf("in child .. ppid is %lu ...and pid is %lu\n", 
		   getppid(),getpid());	   
	   

           //do any work in the child process
	   
           
           continue;  
   }

 }//while

 //this block of code will be executed only by the parent and 
 //parent will reach here only if it has broken the first while loop 
 //and completed its basic work !!!

 //for most cases of process coding, you must use waitpid() as 
 //shown below - in addition, if you observe, this loop is
 //outside the parent's main execution block - meaning, any such
 //clean up activity must be done by the parent after its 
 //actual work - in addition, if you do not code using this 
 //approach, concurrency and multitasking may be lost !!!

 //the code below is a template for the conditions and loop
 //however, modify it as per your understanding and needs !!!
 //the code below is passive - meaning, just prints information 
 //when you code,you may have to take more actions, actively !!! 

 if(ret>0)
 {
   while(1){ //this while(1) is ok - it has been used with a clear purpose
             //it will break when a certain condition is true - see below !!! 
    ret = waitpid(-1,&status,0);
    if(ret>0){

    if(WIFEXITED(status))     //normal termination of the process
    {
       if(WEXITSTATUS(status) == 0){ //normal and successfull
       }
       else{//normal, but not successfull

       }
    }
    else{ //abnormal (did not succeed)
    }
   }//ret>0

    if(ret<0) { exit(0); } //no child is any state for this process
                           //all the children have terminated and 
                           //cleaned-up by the parent process
   } //second while 
  }//if after while loop 

return 0;
   
}


