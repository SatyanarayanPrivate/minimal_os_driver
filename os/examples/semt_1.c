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

    id1 = semget(KEY1, 1, IPC_CREAT|0600);
    //id1 = semget(KEY1, 3, IPC_CREAT|0600);
    if(id1<0) { perror("error in semaphore creation"); exit(1); }    



    //what do we achieve by the end of the following 2 statements ???
    //first semaphore of the semaphore object is initialized to zero !!! 
    //error checking is missing in semctl() - add it ??
    //second parameter represents the index of the semaphore
    //in a semaphore object - if there is only one semaphore, 
    //this is just 0 - otherwise, 0, 1, 2, ,,,,

    //in this context, SETVAL command will interpret the val 
    //field of the union !!! for SETALL command, it may not 
    //interpret val field of the union - it will use array field
    //of the union - refer to other examples !!!

    //based on the val field of the union, initial value of the 
    //0th semaphore is set to 0 
    u1.val = 0;
    semctl(id1,0,SETVAL,u1);

    
    //what do we achieve by the following 2 state ments ??
    //
    //
    //add error checking to semctl() !!
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

     //operate on 0 

    //semop() is the system call used to operate on a semaphore-
    //meaning, decrement operation or increment operation !!!
    sboa[0].sem_num = 0;  //we are operating on the first semaphore
    sboa[0].sem_op = -1;  //the operation is decrement operation 
    sboa[0].sem_flg = 0;  //normally, flags can be set to 0 


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
    sboa[0].sem_flg = 0;

    sboa[1].sem_num = 2;
    sboa[1].sem_op = -1;
    sboa[1].sem_flg = 0;


    semop(id1, sboa, 2);

    semctl(id1,0,GETALL,u1);*/

    printf("the values are %lu .. %lu .. %lu\n", ary1[0],
	   ary1[1],ary1[2]);

   //in this context, second parameter is ignored -
   //meaning, when we delete /destroy a semaphore, 
   //all semaphores of the semaphore object and the
   //semaphore object are destroyed !!!!

    semctl(id1,0,IPC_RMID); //to delete or destroy the 
                            // semaphore object and all
                            // all semaphores in it 
   

}

