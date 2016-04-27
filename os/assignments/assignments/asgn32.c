#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>

#define KEY1 1234

union semun {
             int val;                  /* value for SETVAL */
             struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
             unsigned short *array;    /* array for GETALL, SETALL */
                                      
             struct seminfo *__buf;    /* buffer for IPC_INFO */
       };


int main()

  {

    int ret1, ret2, id1,retf,ret,status,retw,retc;
    union semun u1;
    struct sembuf sb;
      
    id1 = semget(KEY1, 1, IPC_CREAT|0600);
    if(id1<0)
    {
	    perror("error in semaphore creation");
	    exit(1);
    }    

     u1.val=8;
     semctl(id1,0,SETVAL,u1);
   
    retf=fork();
    if(retf<0)
    {
      perror("error in fork!!\n");
     
    }

  if(retf>0)
  {
    printf("in parent process\n");
   
   
    ret = semctl(id1,0,GETVAL);
    printf("3..before increment ..the value of sem 0 is %lu\n",ret);
   
    //u1.val = 0;
    
   // u1.val = 0;
   // semctl(id1,0,SETVAL,u1);
       	    sb.sem_num = 0;
	    sb.sem_op =  +1;
	    sb.sem_flg = 0;

	    semop(id1,&sb, 1);

    ret = semctl(id1,0,GETVAL);
    printf("3..after increment ..the value of sem 0 is %lu\n",ret);
   // semctl(id1,0,IPC_RMID);
  } 
  if(retf>0)
  {
     retw = waitpid(-1,&status,0);
    if(retw>0)
    {

   	 if(WIFEXITED(status))    
   	 {
       	if(WEXITSTATUS(status)==0)
	   {
		   printf("Cleanupin progress for %d\n",retw); 
		   
	   }
       else{
	       printf("normal termintation but not successful\n");

       	   }
   	 }
       else
    	{ 
		printf("abnormal termination\n");
		exit(1);
    	}
    
   }
    if(retw<0)
    {
	    exit(1); 
    } 
  }


    if(retf==0)
    {
      printf("in child process with pid: %d \n",getpid());
	    
    
    ret1 = semctl(id1,0,GETVAL); 
    printf("1..before decrement..the value of sem 0 is %d\n",ret1);
   

    sb.sem_num = 0;  
    sb.sem_op = -1;  
    sb.sem_flg = 0;  
    semop(id1, &sb, 1);
    ret1 = semctl(id1,0,GETVAL); 
    printf("2..after decrement..the value of sem 0 is %d\n",ret1);
    }
return 0; 
}

