#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main()
{

	int ret,status,ret1,i=0,flag=1,flag1=1,ret2,ret3,ret_execl,retw;
   
  while(i++<4)
  {
   ret=fork();
  // printf("the value is %d \n",ret);
 
   if(ret<0)
   { 
     perror("error in fork\n");          
      exit(1); 
   }

   if(ret>0)
   { 
	printf("\nI am in parent process context\n\n"); 
	continue;
        //printf("in parent .. ppid is %lu ...and pid is %lu\n\n\n", getppid(),getpid());
       
        //wrong place for waitpid() 
	//waitpid() must be used outside the first while loop
	//after parent has created all the children processes 
	//refer to fork2n.c for how to use waitpid() outside
	//the first while loop !!!

   }
   if(ret==0) 
   { 
	  // printf("\nI am in child process context\n"); 
           printf("in child .. ppid is %d ...and pid is %d\n", getppid(),getpid());	   
	  switch(i)
	  {

		  //you must compile a C file to generate an object file
		  //you must use gcc with -c option to generate 
		  //an object file 
		  //refer to fork_exec.c, process_unix.txt and unix_env.txt
		  case 1: printf("\n in case1\n");
 			  ret = execl("/usr/bin/gcc","gcc", "-c", "1.c", NULL); 
			  exit(1);  //you must terminate with exit(n), when execl() fails - otherwise, waitpid() may not be
			              //able to extract the exit status code appropriately !!!
			  break;

	  
		  case 2: printf("\n in case2\n");
			  ret = execl("/usr/bin/gcc","gcc", "-c", "2.c", NULL); 
	                  exit(1);
			  break;
			  
		  case 3: printf("\n in case3\n");
			  ret = execl("/usr/bin/gcc","gcc", "-c", "3.c", NULL);
			  exit(1);
			  break;

		  case 4: printf("\n in case4\n");
			  ret = execl("/usr/bin/gcc","gcc", "-c", "4.c", NULL);
			  exit(1);
			  break;
			
		default:
		          break;
	  }

		 
	 }	
   
  }//while loop

  //in addition, follow the comments given in unix_env.txt for 
  //4th problem of assignment 1 - it tells what are the further 
  //actions to be taken !!! 
  //
  //
  //the logic used below is not correct - analyse the code once to modify the 
  //logic appropriately !!!

  //in detail, you must have a default value for a flag and
  //change this value, whenever you find that a child process
  //has terminated normally/unsuccessfully or abnormally 
  //
  //eventually, you will check this flag after cleaning up
  //all the children processes - if all the children processes
  //completed their work normally/successfully, the flag will
  //contain default value - otherwise, it will be changed during
  //cleanup - you must build rest of you logic based on this
  //flag arrangement !!!
  //
  if(ret>0)
{
   while(1)
   {

   ret3 = waitpid(-1,&status,0);
   printf("the val is after wait %d",ret3);
    if(ret3>0)
     {
    	if(WIFEXITED(status))     //normal termination of the process
    	{
	    if(WEXITSTATUS(status) == 0)
	    {
		    printf("\n cleanup pid %d \n", ret3);//normal and successfull
    		   
		   // exit(0);
		    
      	    }
	    else
	    {
		    printf("\n normal but didnt succeed\n");
		    flag=0;
		   // exit(0);
		    
            } 
       	}
	    else
	    {
		    printf("abnormal\n");
		   flag=0;
		    exit(1);
	    }
     }
    if(ret3<0)
    {
	    break;
    }
	 
   }//while 
}//if



if(flag!=0)
{	
 ret1=fork();
           if(ret1<0){
	                    perror("error in this");
	                     exit(1);
                     }
          if(ret1>0){

                     printf("\n\nIN Parent Before linking %lu\n",getpid());
                     retw = waitpid(-1,&status,0);
          	     if(retw>0) 
		     {
                     	printf("child with pid %d cleaned-up\n", retw);
                     	if(WIFEXITED(status))
		     	{
                      	 printf("normal termination and the exit status is %d\n", WEXITSTATUS(status));
		       	
		     	}
                     	else
		     	{
                       	printf("the process terminated abnormally\n");
		     	flag1=0;
		     	}
                     }
                     else
		     {  flag1=0;
                       exit(1);
		     }
                    }

          if(ret1==0)
		{
                        
			ret_execl=execl("/usr/bin/gcc","gcc","-o","linking","1.o","2.o","3.o","4.o",NULL);
		    
		   if(ret_execl<0)
		  	 {
		               perror("\nerror while linking\n");
			       exit(1);
			 }
         	}
}
else
{
	printf("\n ERROR.....so cant link files\n");
	exit(1);
}

if(flag1!=0)
{
ret2=fork();

	if(ret2>0)
	{
		printf("\n\nIn parent...%lu\n",getpid());
		retw = waitpid(-1,&status,0);
          	if(retw>0) 
		{
                     printf("child with pid %d cleaned-up\n", ret2);
                     if(WIFEXITED(status))
		     {
                     	printf("normal termination of child %d and the exit status is %d\n\n",ret2, WEXITSTATUS(status));
		       
		     }
		     else
		     {
			       printf("the process terminated abnormally\n");
		     }
		}
                else
		{
                	exit(1);
		}
	}

	if(ret2==0)
	{
		printf("\nThe Last child created with pid : %lu was created\n",getpid()); 
		ret_execl=execl("./linking","linking",NULL);  //execl() syntax is not proper !!!
		if(ret_execl<0)
		{
			perror("\nERROR cannot launch file\n");
			exit(1);
		}


	}
}
else
{
	printf("\n cannot launch link-file\n");
	exit(1);
}


}//main

 

