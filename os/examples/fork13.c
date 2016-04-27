#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<sched.h>

int main()
{

   int ret,status;

   unsigned long int i=0;

   struct sched_param p1,p2; 


  while(i++<1){ //change this with a reasonable condition !!!

   ret = fork();
 
   if(ret<0){ 
           perror("error in fork"); 
           printf("the final value of i is %lu\n", i);
          
           exit(1); 
   }

   if(ret>0){ 
	   printf("I am in parent process context\n"); 
           printf("in parent .. ppid is %lu ...and pid is %lu\n", 
		   getppid(),getpid());	   
	   
           //++i;     //ok
           continue;
   }

   if(ret==0) { 

           //it only provides the policy of the current process  
           ret = sched_getscheduler(getpid());

           if(ret<0) {
                     perror("error in sched_getscheduler"); 

                     exit(2); 

           }

           printf("1...the value of the scheduling policy is %d\n", ret); 

          ret = sched_getparam(getpid(), &p2);

           if(ret<0) {
                     perror("error in sched_getparam"); 

                     exit(3); 

           }           


           printf("1...the value of the real time priority is %d\n", 
                                           p2.sched_priority);
            //as per the rules of the Linux system ,
           //real time priority must be 1 - 99 
           p1.sched_priority = 5;

           ret = sched_setscheduler(getpid(), SCHED_FIFO, &p1);

           if(ret<0) {
                     perror("error in sched_setscheduler"); 
                     exit(1); 
           } 
          
           //it only provides the policy of the current process  
           ret = sched_getscheduler(getpid());

           if(ret<0) {
                     perror("error in sched_getscheduler"); 

                     exit(2); 

           }

           printf("2...the value of the scheduling policy is %d\n", ret); 

          ret = sched_getparam(getpid(), &p2);

           if(ret<0) {
                     perror("error in sched_getparam"); 

                     exit(3); 

           }  

           printf("2....the value of the real time priority is %d\n", 
                                           p2.sched_priority);

	   printf("I am in child process context\n"); 
           printf("in child .. ppid is %lu ...and pid is %lu\n", 
		   getppid(),getpid());	   
	   

           //do any work in the child process
	   
           //not ok 
           exit(0); 
   }

 }//while

 //this block of code will be executed only by the parent and 
 //parent will reach here only if it has broken the while loop 
 //and completed its basic work !!!

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


