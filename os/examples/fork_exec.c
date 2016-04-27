#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,status,parentid;

   //getpid() is a system call API - such APIs extract information 
   //from system space and copy/return to user-space - in this case,
   //getpid() gets the information from current process pd !!!
   //
   //there are several other system call APIs that may extract 
   //info from pd or nested data objects or other descriptors
   //, in the system !!!
 
   parentid = getpid();

   //what happens in this scenario ??

   //the string is added to the stdio buffer, but  not flushed
   //
   //in addition, during fork(), user-space contents are duplicated
   //for child process - this includes everything in user-space !!
   //stdio buffer is also duplicated !!
   printf("in parent context pid is %d and ppid is %d\n", 
                                    getpid(), getppid()); 
   ret = fork();
   //ret will be +ve, in parent context, if fork() is successful 
   //ret will be -1, in parent context, if fork() fails 
   //ret will be 0, in child context , if fork() succeeds !! 
   //perror can interpret errno value returned by system call API
   //and print appropriate message  appended to user's message !!!
   //do not use printf(), when there are errors - perror() can 
   //print what you give as well as interpret errno variable
   //of the library - the final message is a concatenated 
   //message !!!
   //
   //whenever there is error in a process code, process may 
   //terminate using exit(n) - typical values of n may be 
   //1,2,3,.....till 255, subject to the underlying rules
   // of the system !!!

   if(ret<0){ perror("error in fork"); exit(1); }

   //if(getpid() == parentid){

   if(ret>0){ 
          //any useful code of the parent process can exist here

          printf("in parent context pid is %d and ppid is %d\n", 
                                    getpid(), getppid()); 
           //first parameter is set to -1 such that 
           //waitpid() will clean-up any child process of 
           //this parent process, which is currently in terminated state  !!
           //the first parameter can also be +ve - in this case, 
           //waitpid() will cleanup only the specific child process !!!
           //most common use is to set first parameter to -1 
	   //also refer to manual page of waitpid()  
           //second parameter is used to extract the termination/exit status code
           //of the child process - waitpid() system call API fills the
           //status field when the system call API is invoked !!!
           //last field is the flags field - currently unused - in most 
           //cases, flags field will be set to 0 - meaning, default
           //behaviour of the system call API !!! if you really need to
           //use flags, refer to manual page of waitpid() !!!
           //also refer to process_unix.txt under day3_4_5/
          //this is a simpler usage - refer to other examples for
          // a more thorough usage of waitpid() 
	  ret = waitpid(-1,&status,0); 
           if(ret>0) {
                   printf("child with pid %d cleaned-up\n", ret); 
                   if(WIFEXITED(status))
                     printf("normal termination and the exit status is %d\n", WEXITSTATUS(status));
                   else
                       printf("the process terminated abnormally\n");  
                   //assuming that the parent process has collected the exit status code
                   //of the first child and the compilation is successful, 
                   //create another child process and load the newly created binary 

          }
//           pause(); //used to block a process, unconditionally  !!!
           //sleep(10);   
	   exit(0); 
   }

  else { 
          printf("in child context pid is %d and ppid is %d\n", 
   

                                 getpid(), getppid()); 
	   
   //pause() is the older version of sigsuspend(), which 
   //must not be used in newer code - from here on, stop
   //using pause() and start using sigsuspend() 
   //pause();    //pause() is used for unconditional blocking 
              //of a process !!!
           //we want the child to launch a compiler

   //if vi is launched, this process will start executing vi
   //when vi terminates normally, this child process will terminate !!!

   ret = execl("/usr/bin/vi", "vi", "fork2n.c", NULL);  

   if(ret<0) { perror("error in execl"); exit(33); }     
   //ret = execl("/usr/bin/gcc","gcc","fork1n.c", "-o", "fork1n",NULL);  
   printf("my program is successfully loaded \n"); 
   //error handling must be added 
 

//   pause();    
   exit(0);  //this exit will never run - this is added only for 
             //formality !!!
  }

return 0;
   
}


