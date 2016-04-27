#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>


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

int stat_var;

int main()
{

  char value;
  int ret1,ret2,status, shm_id1,sem_id2;
  struct sembuf sb1,sb2,sb3,sb_array[3];

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

  struct shm_area *shma ;


  printf("the address that faults is %x\n", shma); 

  //shma->buf_size_max = 100; 


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
  shma = shmat(shm_id1,0,0);  //address space is assigned by 
                        //kernel , flags are 0
                        // customize access to shared-area

  

  printf("the attached address is 0x%x\n", shma);
  printf("the amount of shm memory used is %d\n", \
	  sizeof(struct shm_area));
 


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary[0], ary[1], ary[2]); 
  

  //study/use this code along with prod_1.c (this is cons_1.c)
  //however, do not initialize the semaphore in producer and
  //consumer - do it in one of the places and ensure that 
  //that process is executed first !!!

  u1.array = ary1;

  ret1 = semctl(sem_id2,0,GETALL,u1);//getting the semaphore values


  if(ret1<0) { perror("error in semctl getting"); exit(5); }


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary1[0], ary1[1], ary1[2]); 

  
  
 
  
    while(1)
    {
        //consumer

     //in the circular buffer
     printf("entering critical section\n"); 


     if(shma->used_lot_count>0)
     {
      value = shma->buf_area[shma->rd_index];
      printf("the new value is %c\n",value); 
      shma->rd_index = (shma->rd_index+1)%shma->buf_size_max;
    
      //this line is the problem !!!
      //as explained in the previous process, if there is preemption
      //in the middle of this line and the other that uses this     
      //variable is scheduled, inconsistency or race condition may occur 
      //
   
      //decrement operation on semaphore1

      shma->used_slot_count--;

      //increment operation on semaphore1

     }
    printf("exiting  critical section\n"); 
   }//consumer while



   exit(0);

}




