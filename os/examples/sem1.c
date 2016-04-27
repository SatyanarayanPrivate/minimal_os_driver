#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>

#define KEY1 1234

union semun {
             int val;                  /* value for SETVAL */
             struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
             unsigned short *array;    /* array for GETALL, SETALL */
                                       /* Linux specific part: */
             struct seminfo *__buf;    /* buffer for IPC_INFO */
       };


int main()

  {

    int ret1, ret2, id1, id2;
    union semun u1;
    struct sembuf sboa[2];
    unsigned short ary1[3],ary[3] = { 50,0,1};

    //here, we create a semaphore object and a semaphore inside
    //the semaphore object 
    //param1 is the KEY value - must be unique a semaphore 
    //object !!!
    //param2 is the no of semaphores in a semaphore object !!!
    //in a typical semaphore object, there will be only one
    //semaphore - in certain semaphore objects, there may 
    //be multiple semaphores - depends on application and
    //developer's choice !!! once again, these are implementation
    //based - some implementations may not support multiple semaphores
    //in a semaphore object - some may do so !!!

    id1 = semget(KEY1, 1, IPC_CREAT|0600);
    //id1 = semget(KEY1, 3, IPC_CREAT|0600);
    if(id1<0) { perror("error in semaphore creation"); exit(1); }    

 
    //we must initialize the semaphore inside a semaphore object !!!


    

    u1.val = 1;
    

    //first param is the id of the semaphore object !!!
    //second param is the index (identity of the semaphore 
    //                             inside a semaphore object )
    //what is the meaning of param2 here - what does 0 mean ??
    //0 means first semaphore in the semaphore object !!!
 
    //param3 is SETVAL - this command is used whenever we wish 
    //to modify a semaphore's(indicated by param2) value in a 
    //semaphore object(indicated by param1) 
    //
    //finally, we have param3 - union has many fields - based on 
    //the specific field used, a certain initialization operation 
    //is done on one or more semaphores of the semaphore object !!!
    //
    //see above for semaphore union !!!  
    //in our context, we are using SETVAL as the command and 
    //the field of the union that is used is int val - this 
    //field will be containing the initial value to be set for
    //the semaphore indicated in param2 
    //
    //once the system call API succeeds,we can be assured that 
    //a semaphore is initialized to the value passed by the int val 
    //field of the union passed in the param4 !!! 



     semctl(id1,0,SETVAL,u1);
    //semctl(id1,1,SETVAL,u1);

    //fourth parameter in this systemcall API is ignored !!!
    //return value gives the current semaphore value requested
    //via this system call API !!!

    //the system call API below is used to retrieve the current 
    //value of a  semaphore(param2) in the semaphore object(param1)
    //return value of the semaphore is given via the ret variable !!! 
    ret1 = semctl(id1,0,GETVAL); 
    printf("1..before decrement..the value of sem 0 is %lu\n",ret1);
   

    /*u1.val = 50;

    semctl(id1,0,SETVAL,u1); 
    u1.val = 0;
    semctl(id1,1,SETVAL,u1); 
    u1.val = 1;
    semctl(id1,2,SETVAL,u1); 
     

    ret1 = semctl(id1,0,GETVAL); 
    printf("the value of sem 0 is %lu\n",ret1); 
    ret1 = semctl(id1,1,GETVAL); 
    printf("the value of sem 1 is %lu\n",ret1);
    ret1 = semctl(id1,2,GETVAL); 
    printf("the value of sem 2 is %lu\n",ret1);*/

    //either above or below
    //u1.array = ary;
    
    //semctl(id1,0,SETALL,u1);

    //u1.array = ary1;

    //semctl(id1,0,GETALL,u1);

    //printf("the values are %lu .. %lu .. %lu\n", ary1[0],
    //	   ary1[1],ary1[2]);


    //following is the system call API used to operate on 
    //a given a semaphore in a semaphore object 
    //syntax is a little sticky !!!

     //operate on 0 
     //following is also the sembuf {} object's elements 
     //do not confuse sembuf {} object's elements with 
     //semaphore elements of the semaphore object !!!
    sboa[0].sem_num = 0;  //we are operating on the first semaphore
    sboa[0].sem_op = -1;  //the operation is decrement operation 
    sboa[0].sem_flg = 0;  //normally, flags can be set to 0 



    //param1 is the id of the semaphore object 
    // param2 is the ptr to an array of struct sembuf{} elements
    //no elements depends on the requirement and param3 - in our case,
    //we are just using a single element array - meaning, we are using 
    //only one struct sembuf{} - meaning, we are interested in operating 
    //on one semaphore of the semaphore object !!! 

    //in further examples, we will be using an array of multiple
    //elements !!!
    semop(id1, sboa, 1);


    ret1 = semctl(id1,0,GETVAL); 
    printf("2..after decrement..the value of sem 0 is %lu\n",ret1);


    //operate on 0 and 2 
    //sboa[0].sem_num = 0;
    //sboa[0].sem_op = -1;
    //sboa[0].sem_flg = 0;

    //sboa[1].sem_num = 2;
    //sboa[1].sem_op = -1;
    //sboa[1].sem_flg = 0;


    //semop(id1, sboa, 2); 

    /*semctl(id1,0,GETALL,u1);

    printf("the values are %lu .. %lu .. %lu\n", ary1[0],
	   ary1[1],ary1[2]);

    //operate on 0 and 2 
    sboa[0].sem_num = 1;
    sboa[0].sem_op = -1;
    sboa[0].sem_flg = 0;*/

    sboa[1].sem_num = 0;
    sboa[1].sem_op =  +1;
    sboa[1].sem_flg = 0;


    semop(id1, sboa, 1);

    ret = semctl(id1,0,GETVAL);

    printf("3..after increment ..the value of sem 0 is %lu\n",ret);


    //param1 and param3 are typical - param2 is specially 
    //used in this context - meaning, param2 is ignored
    //in this context and entire semaphore object is
    //destroyed !!! 
    
    //such peculiarities are very common in 
    //operating system and system call API !!!

    semctl(id1,0,IPC_RMID); //to delete or destroy the 
                            // semaphore object and all
                            // all semaphores in it 
    










}

