#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<stdio.h>

#define KEY1 1234

struct shm_area {
  unsigned short rd_index;
  unsigned short wr_index;
  unsigned short buf_size_max;
  int buf_area[50]; //this must be treated as circular buffer
  unsigned int used_slot_count;

  //char my_straread[50][50];
};

union semun {

  int val;
  // other fields are omitted

};

int stat_var;

int main()
{

  char value;
  int ret1,ret2,shm_id1,sem_id2;
  struct sembuf sb1,sb2,sb3,sb_array[3];

  // first semaphore(0) will be used for counting free slots
  //
  // second semaphore is used for mutual exclusion 
  //
  // third semaphore is used for counting filled slots 
  //
  unsigned short ary[] = { 50,1,0};  

  union semun u1;

  struct shm_area *shma;

  shm_id1 = shmget(KEY1,sizeof(struct shm_area),0);
                                         //the last field may be 0 ,
              				 //when the shared memory object
                                         //is already created 
  if(shm_id1<0 && errno != EEXIST) {   }

  sem_id2 = semget(KEY1,3,0);            //last field may or may not be 0

  shma = shmat(shm_id1,0,0);  //address space is assigned by 
                        //kernel , flags are 0
                        // customize access to shared-area

  printf("the attached address is 0x%x\n", shma);
  printf("the amount of shm memory used is %d\n", \
	  sizeof(struct shm_area));
  
   //producer
   //
   while(1)
   {
    
    //read(STDIN_FILENO,&value,1);  //this is good programming
    //printf("the value is %c\n", value);
    //decrement freeslot counting semaphore using semop
    //and allocating a free semaphore
    //sb1.sem_num = 0;  //semaphore instance no.
    //sb1.sem_op = -1;  //decrement operation 
    //sb1.sem_flg = 0;  //not using flags
    //semop(sem_id2,&sb1,1);  
     
    
    //decrement mutual exclusion semaphore using semop
    //entering critical section / atomic section
    //sb2.sem_num = 1;
    //sb2.sem_op = -1;
    //sb2.sem_flg = 0;
    //semop(sem_id2,&sb2,1); 
   
    //two semaphore operations done atomically
    //
    sb_array[0].sem_num = 2;
    sb_array[0].sem_op = -1;
    sb_array[0].sem_flg = 0;

    sb_array[1].sem_num = 1;
    sb_array[1].sem_op = -1;
    sb_array[1].sem_flg = 0;
 
    semop(sem_id2,sb_array,2);   //two operations 

    // read(STDIN_FILENO,&value,1); //this is bad programming
    //add an item to the next free slot in 
    //in the circular buffer
    value = shma->buf_area[shma->rd_index];
    shma->rd_index = (shma->rd_index+1)%shma->buf_size_max;

    //two semaphore operations done atomically
    //
    sb_array[2].sem_num = 0;
    sb_array[2].sem_op = +1;
    sb_array[2].sem_flg = 0;

    sb_array[1].sem_num = 1;
    sb_array[1].sem_op = +1;
    sb_array[1].sem_flg = 0;
 
    semop(sem_id2,&sb_array[1],2);   //two operations

    //increment  mutual exclusion semaphore using semop
    //we are leaving the critical section
    //sb2.sem_num = 1;  //we are operating on the second semaphore
    //sb2.sem_op = +1;  //increment
    //sb2.sem_flg = 0;
    //semop(sem_id2,&sb2,1); 

    //increment filled slot counting semaphore using semop
    //
    //sb1.sem_num = 2;
    //sb1.sem_op = +1;
    //sb1.sem_flg = 0;
    //semop(sem_id2,&sb1,1);

    //sched_yield();  //choice
   }//loop	  

exit(0);

}




