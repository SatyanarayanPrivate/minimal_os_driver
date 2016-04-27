#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>


#define KEY1 1234


//in the below example, we are implementing an user-space pipe
//mechanism - for those who are familiar with Unix/other operating
//systems, system space pipe ipc is very popular !!!
//
//we will be studying and using system space pipes after 
//working with file system /related system call APIs 
//in user-space pipe and system space pipe, the ipc buffer used 
//to store/forward data is a circular buffer - you will understand
//the implementation, if you look into the code below !!! 

//following is the user-space pipe object - you may modify 
//this pipe object as per your requirements - however, 
//certain basic fields must stay !!!
struct shm_area {
  unsigned short rd_index;  //read index for the circular buffer
  unsigned short wr_index;  //write index for  ""   """

  unsigned short buf_size_max; //max buffer size 
  char buf_area[50]; //this must be treated as circular buffer
  unsigned int used_slot_count;//no elements used in the circular buffer 

  //char my_straread[50][256];   //practice
};

union semun { //used with semctl() system call for initializing 
              //getting the current semaphore count values
  int val; //this will be used with SETVAL command 
  unsigned short *array; //this will be used with SETALL command
  // other fields are omitted

};

int stat_var;

int main()
{

  char value;
  int ret1,ret2,status, shm_id1,sem_id2;
  struct sembuf sb1,sb2,sb3,sb_array[3];

  //in this assignment, 3 semaphores are used in a semaphore object 
  //the usage of the semaphores are as below :

  // first semaphore(0) will be used for counting free slots
  // in the IPC area  - this is a counting semaphore type -
  // as discussed in ipc.txt,for more details refer to ipc.txt  
  //
  // second semaphore is used for mutual exclusion 
  // in the IPC area 
  // refer to ipc.txt - this is a binary semaphore - as 
  // decided by the developer !!! this semaphore is responsible 
  //for locking, in this example context !!!

  //
  //
  // third semaphore is used for counting filled slots 
  // in the IPC area - this is again a counting semaphore -
  // refe to ipc.txt 
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

  //we are creating 3 semaphores in a given semaphore object !!!
  //in this implementation, a semaphore object can have n no. of 
  //semaphores - n is defined by the developer - in this context,
  //we are creating 3 semaphores !!!
  //param2 is set to 3 such that 3 semaphores are created in 
  //a semaphore object !!!


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

  //internally, this will generate VADs, new page table entries
  //and connect VADs to shared memory object - all these objects
  //work together to ensure that shared memory mechanism works
  //appropriately !!!


  //by collecting the address in a pointer of pipe object type,
  //we are treating the shared memory area as a functional pipe !!!

  shma = shmat(shm_id1,0,0);  //address space is assigned by 
                        //kernel , flags are 0
                        // customize access to shared-area

  

  printf("the attached address is 0x%x\n", shma);
  printf("the amount of shm memory used is %d\n", \
	  sizeof(struct shm_area));
 


  printf("the actual values of the semaphores are %d, %d, %d\n", \
                    ary[0], ary[1], ary[2]); 
 //semctl() can be used with several commands - SETVAL is a command
 //used to set a single semaphore's value inside a semaphore object !!!
 //semctl() can also be used with SETALL - SETALL is a command 
 //used to set values of all semaphores, in a semaphore object !!!!
 //when you use SETALL with semctl(), second parameter is ignored -
 //meaning, you cannot use SETALL to initialize a semaphore !!! 
 // 

 //refer to man semctl for more details on the second parameter as well
 //as other parameters !!!
 //
 //when semctl() is used with SETALL command, initial values of all 
 //the semaphores in the semaphore object will be initialized !!! 
 //sem_id2 is the first parameter
 //second parameter is ignored 
 //third parameter is the command to set/initialize
 // the values of all semaphores
 //in the semaphore object !!!
 //fourth param is pointer to an array of unsigned short elements-
 //each unsigned short element contains corresponding semaphore's
 //initial value - no of elements in this array must equal to
 //total no of semaphore elements in the semaphore object
 //array field of union is used for SETALL command - array field 
 //is used to point to an array of unsigned short elements - 
 //each element in the array initializes one semaphore in the semaphore
 //object !!! 

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
  
   //if you understand the below code and analyse it, you will
   //conclude that the code may enter a busy waiting state - 
   //do not confuse this will spin lock - this has nothing to 
   //do with spinlocks 
   //such a problem in user space is known as busy-waiting 
   //and this leads to inefficient use of processor cycles !!!

 
   //producer
   //
   while(1)
   {
    
    //wait for user input and read the user-input
    //it is a blocking system call
    //
    ret = read(STDIN_FILENO,&value,1);  //this is good programming
    printf("the value is %c\n", value);
    //decrement freeslot counting semaphore using semop
    //and allocating a free semaphore
    //sb1.sem_num = 0;  //semaphore instance no.
    //sb1.sem_op = -1;  //decrement operation 
    //sb1.sem_flg = 0;  //not using flags
    //semop(sem_id2,&sb1,1);  
     
    //semop() is better explained in day6_7/ipc.txt    
    //decrement mutual exclusion semaphore using semop
    //entering critical section / atomic section
    sb2.sem_num = 1;
    sb2.sem_op = -1;
    sb2.sem_flg = 0;
    semop(sem_id2,&sb2,1); 
   
    //two semaphore operations done atomically
    //
    //sb_array[0].sem_num = 0;//preparing the decrement operation on 
    //sb_array[0].sem_op = -1;//semaphore 0 in the sem.object - 
                            //meaning, use a free slot in producer
    //sb_array[0].sem_flg = 0;

    //sb_array[1].sem_num = 1;   //decrement critical section semaphore 
    //sb_array[1].sem_op = -1;   //acquire semaphore lock !!!
    //sb_array[1].sem_flg = 0;
 
    //semop(sem_id2,sb_array,2);   

    // read(STDIN_FILENO,&value,1); //this is bad programming
    //add an item to the next free slot in 
    //in the circular buffer
    if(shma->used_slot_count < shma->buf_size_max)
    {
       shma->buf_area[shma->wr_index] = value;
       shma->wr_index = (shma->wr_index+1)%shma->buf_size_max;
       shma->used_slot_count++;
    }
    //two semaphore operations done atomically
    //
    //sb_array[2].sem_num = 1;  //critical section semaphore
    //sb_array[2].sem_op = +1;  //releasing critical section semaphore  
    //sb_array[2].sem_flg = 0;

    //sb_array[1].sem_num = 2;  //incrementing filled slots semaphore count 
    //sb_array[1].sem_op = +1;  //meaning, this semaphore maintains the count
                              //of filled slots - this is use ful in the
                              //context of consumer 
    //sb_array[1].sem_flg = 0;
 
    //semop(sem_id2,&sb_array[1],2);   

    //increment  mutual exclusion semaphore using semop
    //we are leaving the critical section
    sb2.sem_num = 1;  //we are operating on the second semaphore
    sb2.sem_op = +1;  //increment
    sb2.sem_flg = 0;
    semop(sem_id2,&sb2,1); 

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




