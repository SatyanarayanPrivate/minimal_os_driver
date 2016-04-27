#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

   int ret,i=0;

   //since the parent process and the child process execute the same
   //piece of code(program), the child will also be having fork() in
   //its code - but, the fork() will not execute fully, but partially
   //and return 0 when the child process is scheduled sometime in the
   //future 
   //
   //

  while(i++<5) //this is how, you start !! this coding is not complete !!!
  {

   ret = fork();
 
   if(ret<0){ perror("error in fork"); exit(1); }

   //the return value ret is a +ve no. in the parent process
   //  
   if(ret>0){ 
	   

	   //while(1){ 

//a \n before a string will not flush the string from library buffer
//a \n after a string will flush the string from library  buffer
//if a new process is created and there is a unflushed string in 
//the library buffer, it will get copied/duplicated into the 
//library buffer of the child process as well !!! although the message
//is meant for parent only, child gets this message, accidentally !!
//
//if you do not flush using \n, many times the string may not 
//reach the screen, due to various problems !!!



                 printf("P\n");
	   printf("I am in parent process context\n"); 
           printf("in parent .. ppid is %d ...and pid is %d\n", 
		   getppid(),getpid());	   

          // }
	   
	   
	   exit(0); 
   }

   if(ret==0) { 
	   

 
	   //while(1){ 

               printf("C\n");


	   printf("I am in child process context\n"); 
           printf("in child .. ppid is %d ...and pid is %d\n", 
		   getppid(),getpid());	   

           //}
	   exit(0); 
   }

  {//while loop ends here !!

return 0;
   
}


