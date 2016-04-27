#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<signal.h>

#define KEY1 1234

struct shm_area {
  unsigned short rd_index;
  unsigned short wr_index;
  unsigned short buf_size_max;
  char buf_area[50]; //this must be treated as circular buffer
  unsigned int used_slot_count;

  //char my_straread[50][256];   //practice
};

union semun { //used with semctl() system call for initializing 
              //getting the current semaphore count values
  int val;
  unsigned short *array;
  // other fields are omitted

};

int stat_var,ret=0;
struct sigaction act1;


void segv_hdl(int sig)
{

 write(STDOUT_FILENO, "we are in signal handler\n", 25); 

}



int main()
{

  char value;
  int ret1,ret2,status, shm_id1,sem_id2;
  struct sembuf sb1,sb2,sb3,sb_array[5];

  // first semaphore(0) will be used for counting free slots
  // in the IPC area
  //
  // second semaphore is used for mutual exclusion 
  // in the IPC area 
  //
  // third semaphore is used for counting filled slots 
  // in the IPC area 
  //
  unsigned short ary1[3],ary[] = { 50,1,0};  

  union semun u1;

  

  struct shm_area *shma = 0x00000000;
  printf("the address that faults is %x\n", shma); 
  //pause();
  act1.sa_handler = segv_hdl;
  act1.sa_flags = 0;
  sigfillset(&act1.sa_mask);
  //sigaction(SIGSEGV,&act1,NULL);  

  //shma->buf_size_max = 50; 

  /*act1.sa_handler = segv_hdl;
  act1.sa_flags = 0;
  sigfillset(&act1.sa_mask);
  sigaction(SIGSEGV,&act1,NULL); */


  //printf("the address that faults is %x\n", shma); 



  shm_id1 = shmget(KEY1,sizeof(struct shm_area),IPC_CREAT|0600);
                                         //the last field may be 0 ,
              				 //when the shared memory object
                                         //is already created 
  if(shm_id1<0 && errno != EEXIST) { 

              perror("error in creating the shared memory"); exit(1);
  }

  sem_id2 = semget(KEY1,3,IPC_CREAT | 0600);//with read/write permissions 

  if(sem_id2<0) {

                perror("error in semget");
                exit(2);
  }

  //last field may or may not be 0

  //add error checking

  //shma will now hold the starting virtual address of the 
  //virtual pages allocated for the shared-memory region
  //
  //what is the attachement that is being done here
  //between the process,VAD and shared memory object ???

  shma = shmat(shm_id1,0,0);  //address space is assigned by 
                        //kernel , flags are 0
                        // customize access to shared-area

  

  printf("the attached address is 0x%x\n", shma);
  printf("the amount of shm memory used is %d\n", \
	  sizeof(struct shm_area));
 


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary[0], ary[1], ary[2]); 
  
  u1.array = ary; //setting the array ptr in the union

  ret1 = semctl(sem_id2,0,SETALL,u1);//setting the semaphore values

  if(ret1<0) { perror("error in semctl setting"); exit(4); }

  u1.array = ary1;

  ret1 = semctl(sem_id2,0,GETALL,u1);//getting the semaphore values


  if(ret1<0) { perror("error in semctl getting"); exit(5); }


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary1[0], ary1[1], ary1[2]); 

  
  shma->rd_index = 0; 
  shma->wr_index = 0;
  shma->buf_size_max = 50;  
  shma->used_slot_count = 0; 
  
  printf("the address  is %x\n", shma); 

  //pause();  
   //producer
   //
   while(1)
   {
    //wait for user input and read the user-input
    //it is a blocking system call
    //

    //have you worked with a physical unix terminal ???
    //read() is a system call API that is used with active files
    //in this case,we are accessing active file of a virtual terminal 
    //STDIN_FILENO is the active file index corresponding to the
    //current virtual terminal !!!  
    //second param is pointer to buffer
    //third param is size of the buffer 
     
    //read() system call API is actually a file system related 
    //system call API !!!wait for file system !!!
    //also read the manual page 

    ret = read(STDIN_FILENO,&value,1);  //this is good programming
    printf("the value is %c\n", value);
    //in the circular buffer

    //here, assume that shma points to a set of shared 
    //virtual pages !!! we are accessing contents 
    //in the virtual pages as per our requirements !!!
    if(shma->used_slot_count != shma->buf_size_max)
    {
    shma->buf_area[shma->wr_index] = value;
    shma->wr_index = (shma->wr_index+1)%shma->buf_size_max;

    //the line that is right below this comments is the problem !!!
    //problem will occur, if this process is preempted in 
    //the middle of executing the line below and the other
    //process is scheduled and the newly scheduled process
    //is also accessing this variable which is a shared variable !!!

    //solution 1 - disable preemption  for this process, when
    //it is executing the following C construct - meaning,
    //below code must be executed atomically - meaning, during 
    //the execution of all machine instructions of the below code,
    //no preemption must occur - in other words, all machine 
    //instructions of below code must execute atomically !!!
    //we cannot implement preemption disabling directly - there
    //is no such feature, normally - there may be such features,
    //in a specialize operating system like RTOS - or, you may 
    //have such facilities , in system space - if finding a direct 
    //facility of disabling preemption is not possible , in user-space 
    //such requirements are satisfied by locks and other mechanisms !!!
    //we will see more details during RTOS study !!!
   

    //the most common mechanism that may be used is known 
    //as semaphore mechanism - let us understand the semaphore mechanism
    //and use it solve the racecondition, in this context !!!


    //decrement operation on semaphore1

     shma->used_slot_count++; 
    
    //increment operation on semaphore1 


    }
   }//loop	  


   exit(0);

}




