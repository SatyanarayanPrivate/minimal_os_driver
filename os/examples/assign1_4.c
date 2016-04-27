#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>

int main()
{

   int ret,pid,status,flag_unsuccessful=0,flag_abnormal=0;
   unsigned long int i=0;

    pid=getpid();
    printf("processid  = %d\n",pid);


  while(i++<5){
   ret = fork();
   
   if(ret<0)
   { 
           perror("error in fork"); 
           printf("the final value of i is %lu\n", i);
           exit(1); 
   }

   if(ret>0)
   { 
	   printf("I am in parent process context\n"); 
           printf("in parent .. ppid is %lu...and child is %lu\n\n", 
		   getppid(),getpid());
   }   

   if(ret==0) 
   { 
	   printf("I am in child process context\n"); 
         	   
	if(i==1)
	{
	printf("in child context pid is %d and ppid is %d\n",getpid(), getppid()); 
	   
	ret = execl("/usr/bin/gcc","gcc","-c","a1.c","-o","a1.o",NULL);  
  	if(ret<0) { perror("error in execl"); exit(33); }       
	}
	
	if(i==2)
	{
	printf("in child context pid is %d and ppid is %d\n",getpid(), getppid()); 
	   
	ret = execl("/usr/bin/gcc","gcc","-c","a2.c","-o","a2.o",NULL);  
  	if(ret<0) { perror("error in execl"); exit(33); }       
	}
	if(i==3)
	{
	printf("in child context pid is %d and ppid is %d\n",getpid(), getppid()); 
	   
	ret = execl("/usr/bin/gcc","gcc","-c","a3.c","-o", "a3.o",NULL);  
  	if(ret<0) { perror("error in execl"); exit(33); }     
	}
	if(i==4)
	{
	printf("in child context pid is %d and ppid is %d\n",getpid(), getppid()); 
	   
	ret = execl("/usr/bin/gcc","gcc","-c","a4.c","-o","a4.o",NULL);  
  	if(ret<0) { perror("error in execl"); exit(33); }       
	}
	if(i==5)
	{
	printf("in child context pid is %d and ppid is %d\n",getpid(), getppid()); 
	   
	ret = execl("/usr/bin/gcc","gcc","-c","a5.c","-o","a5.o",NULL);  
  	if(ret<0) { perror("error in execl"); exit(33); }      
	}
   }
  } //while loop ends here 

 if(ret>0)
 {
   while(1)
   {

     ret = waitpid(-1,&status,0);
     if(ret>0)
     {
            printf("child with id %d terminated\n",ret);

       if(WIFEXITED(status))
       {
            printf("wifexited\n");
           if(WEXITSTATUS(status) == 0)
           { 
              printf("wexitstatus\n");       
           }
           else
           {
               flag_unsuccessful = 1;
               printf("unsuccessful\n");
           }
       }
       else
       {
              flag_abnormal = 1;
       }
     }

     if(ret<0)
     {   
        if(flag_unsuccessful || flag_abnormal) {printf("error in compiling an object file\n"); exit(1); }
        else break;  
     }

   }//second while

  }//second parent block if

  //you must fork and create another child only after checking that 
  //every exit status info of every  child process is normal and 
  //successful - otherwise, you cannot link the object files, if
  //there is error in one object file's compilation !!!
  //use waitpid() to check the exit status code of every child process
  //here and then proceed, if all are successfully terminated !!!
  
  

   ret = fork();
   
   if(ret<0)
   { 
           perror("error in fork"); 
           printf("the final value of i is %lu\n", i);
           exit(1); 
   }
   if(ret>0)
   { 
	   printf("I am in parent process context\n"); 
           printf("in parent .. ppid is %lu...and child is %lu\n\n", getppid(),getpid());
   }

   if(ret==0) 
   { 
	   printf("I am in    COMBINING     child process context\n"); 
           printf("in child .. ppid is %lu ...and pid is %lu\n",getppid(),getpid());           	   
	   ret = execl("/usr/bin/gcc","gcc","a5.o","a4.o","a3.o","a2.o","a1.o","-o", "final",NULL);  
  	   if(ret<0) { perror("error in execl"); exit(33); }      
   }

 flag_unsuccessful = 0;
 flag_abnormal =0; 

 if(ret>0)
 {
   while(1)
   {

     ret = waitpid(-1,&status,0);
     if(ret>0)
     {
            printf("child with id %d terminated\n",ret);

       if(WIFEXITED(status))
       {
            printf("wifexited\n");
           if(WEXITSTATUS(status) == 0)
           { 
              printf("wexitstatus\n");       
           }
           else
           {
               flag_unsuccessful = 1;
               printf("unsuccessful\n");
           }
       }
       else
       {
	      if(flag_unsuccessful || flag_abnormal) {printf("error in compiling an object file\n"); exit(1); }
              flag_abnormal = 1;
       }
     }

     if(ret<0)
     {   
        break;  //you must check conditions properly   
     }

   }//second while

  }//second parent block if 



 //you must verify that the linking child process completed successfully and
 //then go far loading/launching the compiled new program
   

 ret = fork();

       
   if(ret<0)
   { 
           perror("error in fork"); 
           printf("the final value of i is %lu\n", i);
           exit(1); 
   }

   if(ret>0)
   { 
	   printf("I am in parent process context\n"); 
           printf("in parent .. ppid is %lu...and child is %lu\n",getppid(),getpid());

	   
   }

   if(ret==0) 
   { 
	   printf("I am in   RUNNING  child process context\n"); 
           printf("in child .. ppid is %lu ...and pid is %lu\n",getppid(),getpid());	   
           ret=execl("/usr/bin/final","final",NULL);  //wrong  !!!
           //ret=execl("/home/dssd/final","final",NULL);  //correct !!!
           if(ret<0) { perror(" my error in execl"); exit(33); }	   
   }


//the below code must be before creating children for linking and loading 
//compiled program !!! see the the comments above !!!


 
  //sleep(10);


 return 0;
   
}

