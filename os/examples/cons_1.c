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
  



  u1.array = ary1;

  ret1 = semctl(sem_id2,0,GETALL,u1);//getting the semaphore values


  if(ret1<0) { perror("error in semctl getting"); exit(5); }


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary1[0], ary1[1], ary1[2]); 
  
    //if you follow the code below, it will execute consistently 
    //without race conditions - however, it will end up busy waiting 
    //wasting time slices - another good example for bad multitasking !!!

    while(1)
    {
        //consumer

    //two semaphore operations done atomically
    //
    //sb_array[0].sem_num = 2;  //third semaphore is a counting semaphore for filled slots
    //sb_array[0].sem_op = -1;
    //sb_array[0].sem_flg = 0;

    //sb_array[1].sem_num = 1;   //second semaphore is a critical section semaphore 
    //sb_array[1].sem_op = -1;
    //sb_array[1].sem_flg = 0;
 
    //semop(sem_id2,sb_array,2);   

    sb_array[1].sem_num = 1;   //second semaphore is a critical section semaphore 
    sb_array[1].sem_op = -1;
    sb_array[1].sem_flg = 0;
 
    semop(sem_id2,&sb_array[1],1);  
    // read(STDIN_FILENO,&value,1); //this is bad programming
    //add an item to the next free slot in 
    //in the circular buffer
    if(shma->used_slot_count != 0)
    {
      value = shma->buf_area[shma->rd_index];
      printf("the new value is %c\n",value); 
      shma->rd_index = (shma->rd_index+1)%shma->buf_size_max;
      shma->used_slot_count--;
    }
    //two semaphore operations done atomically
    //
    //sb_array[2].sem_num = 1;  //critical section semaphore
    //sb_array[2].sem_op = +1;
    //sb_array[2].sem_flg = 0;

    //sb_array[1].sem_num = 0;
    //sb_array[1].sem_op = +1;
    //sb_array[1].sem_flg = 0;
 
    //semop(sem_id2,&sb_array[1],2);   


    sb_array[2].sem_num = 1;  //critical section semaphore
    sb_array[2].sem_op = +1;
    sb_array[2].sem_flg = 0;

 
    semop(sem_id2,&sb_array[2],1);  
   }//consumer while



   exit(0);

}




