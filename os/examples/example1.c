#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <signal.h> 

sigset_t s1,s2;

struct sigaction act1,act2;


void sig_hdl1(int signo)  //this handler is used to setup custom
                          //action for SIGTERM 
{
  printf("a dummy signal handler1\n");
  //exit(0); 

}  
void sig_hdl2(int signo) //this handler is used to setup custom 
                         //action for SIGINT 
{
  printf("a dummy signal handler2\n");
  kill(getpid(),SIGKILL); 

}

int main(int argc, char *argv[]) 
 { 

     int ret;
     sigset_t set1,set2;

     

     if (argc != 2) 
         exit(0); 
 
    // set every bit in this user-space mask field - this 
    //mask field will be manipulated and passed to sigprocmask()
    //sigprocmask() will use the bit map field that we passed 
    //and the bitmap infomation is copied into signal mask field
    //of the pd !!
    
     //sigfillset is used to manipulate user-space bit map, not system space
     //bit map !! sigfillset() and many related APIs are libraryAPIs!!!
     sigfillset(&set1);  //every bit in this bit map is set !!!
     // sigdelset(&set1, SIGTERM); 

     //sigprocmask() is a system call API - it is used to manipulate
     //signal mask field of the process descriptor !!!
     //first field is a command - SIG_SETMASK is used to tell the
     //system to modify the signal mask field of the current process
     //using other parameters passed to sigprocmask() 

     //second field is the bit map used to set the signal mask field
     //of the current process - contents of set1 are literally copied
     //into the signal mask field of the current process - 
     //in this case, we are setting every bit of signal mask field of the
     //current process  (based on the way we setup set1 in the 
     //                  previous step )
     //- meaning,every signal is masked or blocked !!!  
     //third field is used to copy the old signal mask field from 
     //the pd to user-space variable, which is the third field - it 
     //is done for backing up the current mask field !!! third 
     //field is optional !!!  read man page of sigprocmask() 

     //after sigprocmask() executes, all signals are blocked/masked,
     //but SIGKILL and SIGSTOP 
     sigprocmask(SIG_SETMASK, &set1,&set2) ;

     //we are resetting bit corresponding to SIGQUIT in set1 !!!
  
     sigdelset(&set1, SIGQUIT);


     //what is the effect, if we do this ???
     //    - SIGQUIT is unmasked/unblocked and others will 
     //      be blocked 
     sigprocmask(SIG_SETMASK,&set1, &set2); 

     printf("we are blocked in the first sigsuspend\n"); 

     //sigsuspend(&set1); //block until an unmasked signal arrives

   

     //the bit corresponding to SIGTERM is reset in the user-space
     //bit-map set1 only
     //does not affect the signal mask field in the process descriptor
     //we will look at this modified mask below, as needed !!!

     sigdelset(&set1, SIGTERM); 
     sigdelset(&set1, SIGINT); 
     

     size_t mb = strtoul(argv[1], NULL, 0); 
 
     // Allocate the memory 
     size_t nbytes = mb * 0x100000;

     printf("the actual value is %lu\n", nbytes); 
 
     //malloc() uses a system call, internally - 
     //it is known as mmap() system call - infact, 
     //mmap() system call uses virtual memory management techniques !!
     //what ever we discussed applies to mmap() also - you may 
     //directly use mmap() or indirectly use mmap() !!!
     //internally, mmap() system call API will create a new set
     //of dynamic virtual addresses !! in order to do so, a new 
     //set of VADs and associated page tables are created - in addition, 
     //ptes for secondart page table entries will be set to 0 - invalid !!! 
     //
     //this means, malloc() does not allocated physical memory - it 
     //allocates virtual address space / may be known as virtual memory !!!
     //mmap() internally returns a virtual address to malloc() and malloc()
     //returns the same to the developer !!!

     char *ptr = (char *) malloc(nbytes); 
     if (ptr == NULL) { 
         perror("malloc"); 
         exit(EXIT_FAILURE); 
     } 

     //not all fields of struct sigaction { } are used - 
     //in our case, we use sa_handler field, sa_flags
     //and sa_mask - others are unused - refer to 
     //man 2 sigaction for more details !!!


     act1.sa_handler = sig_hdl1;  //ptr to our signal handler, in user space !!!
     act1.sa_flags = 0;  //currently, we ignore the flags
                         //in most system call APIs, flags are unused and
                         //set to 0 - if you really need to know more 
                         //about flags, refer to manual page !!!
 

    //there is another mask used during setting up signal handler-
    //this mask is valid during signal handler execution only -
    //for example, we may be interested in masking all signals
    //during execution of  a signal handler - this mask field
    //is not used by the system outside this signal handler - 
    //during other times, other signal mask fields may be used -
    //it is as per context !!! 
    
    //what do i achieve by setting this mask field ???
    //in this case, when this handler is executing, all signals
    //are masked and this handler will not be disturbed - this
    //enables this handler to execute atomically with respect 
    //to other signal handlers !!!
    sigfillset(&act1.sa_mask);

    //sigaction() is the system call API that really sets up 
    //the signal action table entry for a specific signal 
    //we can repeat sigaction() call for each signal that we 
    //wish to set up customized signal action 

     //act2 is the parameter used to back-up the previous action
     //for this signal in this process
     //act1 is the parameter used to set the new action for this
     //process, as mentioned in the fields of act1

     //the name is misleading - in this case, all parameters 
     //stored in act1 will just be copied into the corresponding
     //entry in the signal action table of the process/pd !!!
     //nothing more will be done - real action will occur 
     //only when the corresponding signal is generated and 
     //system is able to take action and deliver the signal !!!

     sigaction(SIGTERM,&act1,&act2); 

     act1.sa_handler = sig_hdl2;
     act1.sa_flags = 0;  //currently, we ignore the flags
     sigfillset(&act1.sa_mask);
     sigaction(SIGINT,&act1,&act2);

     //if this process blocks in sys_sigsuspend(), what will happen,
     //  when a SIGTERM is generated for this process ??
     //this process will be woken up and SIGTERM bit 
     //in the signal pending field of this process is 
     //set !!!
     //since this process is blocked in sys_sigsuspend(), after wakeup,
     //this process will resume at some point in sys_sigsuspend() -
     //as part of any system call return, pending signals will 
     //be scanned and actions will be taken 
     //in our case, SIGTERM is the pending signal and action 
     //will be as per the settinsg  in the signal action table 
     //of this process !!!
     //
     //since we have set up a signal handler, system will jump to 
     //the handler and execute it in user-space, not in system space
     //but doing so has a problem - we need to save the original user-space
     //context of sigsuspend() system call API, which was saved in 
     //system stack, when sigsuspend() system call API was invoked !!!
     //
     //once signal handler is executed in user-space, system will force
     //the process to enter system space using a special system call API -
     //this is a trick managed by operating system !!! in fact, operating
     //system is full of such tricks !!!
     //
     //once signal handler is completed and process enters system space,
     //it will use the original user-space context and return from sigsuspend()
     //system call API - in this case, ret will be -1 and errno will be set 
     //to EINTR
     //after this, process will continue executing after sigsuspend()
     //,in user-space as before !!!
     //if you are wondering, why all this ?? two reasons :
     //one is to execute a signal handler
     //second is to finish the signal handler and continue the process
     //in user-space - if you can find a better solution for the above
     //requirement, you may do so !!!
 
 
 
     //sigsuspend() is a system call API that is used to block a process
     //such that it can be woken up, if a signal is generated for this
     //process - a signal will be generated for an event - a signal 
     //generated on behalf of an event and this leads to wake-up of a process !!!  
     //
     //sigsuspend() blocks until an unmasked signal in the mask
     //passed to it arrives - while the sigsuspend() is blocked,
     //the signalmask used by the process is the one that is
     //passed to sigsuspend, not its original signal mask -
     //when sigsuspend() wakes-up, the process starts using 
     //its original signalmask field  
     ret = sigsuspend(&set1);//this process normally blocks here 

     //ideally, if sigsuspend() is blocked and a signal arrives, blocked
     //process will be woken up and terminated due to default action - however,
     //this behaviour can be modified by installing a signal handler -
     //in this case, we are using a dummy signal handler to resume this process
     //to continue executing after sigsuspend(), when SIGTERM signal arrives !!!
     //

     //if a handler is installed, sigsuspend() will return with
     //a -1 in ret and errno set to EINTR

     //for any system call API, system returns 0, +ve or -1 via
     //ret value - in addition, system library may also set 
     //a special library variable known as errno - this variable
    //is valid and relevant when a system call API returns -1 
    //system call APIs return -1, when they have encountered 
    //an error - otherwise, they always return 0 or +ve no 
    //which means, if ret value is -1, errno is valid - it provides
    //the exact error code - other wise, if ret is 0 or + ve ,
    //errno is invalid and must be investigated !!!
    //let us say that a process is blocked in a system call API and
    //a signal handler is installed - if the signal arrives and 
    //action is taken, signal handler will execute and the system 
    //call API will return -1 as well - in addition errno of this
    //process will be set to EINTR by system library !!!
    //however, sigsuspend() is a very special system call API - 
    //if it ever returns, it will return -1 and errno will be
    //set to EINTR !!!



     //we are accessing each virtual page of dynamically allocated memory 
     //for this process - what will happen when we access each virtual 
     //page as we do below ??? there will be page faults aind page frames
     //will be allocated !!!

     //sysconf() is a system call API that can be used to access system 
     //information - the system information that we are accessing is
     //page size !!! refer to manual page of sysconf() for more details !!!
     printf("the starting address of the mem. is %x\n", (unsigned int)ptr); 
     // Touch the memory (could also use calloc()) 
     size_t i; 
     const size_t stride = sysconf(_SC_PAGE_SIZE); 
     for (i = 0; i < nbytes; i += stride) { 
       ptr[i] = 0; 
     } 

     sigfillset(&set1);
     sigprocmask(SIG_SETMASK, &set1,NULL) ; 
 
     printf("allocated %d mb\n", mb); 
     
     //DO NOT USE pause() - use the newer sigsuspend()
     //pause();   //this is a system call that will block the process temporarily
                //until a signal arrives
                //if a signal arrives and there is no signal handler installed,
                //the process will wake-up and the default action will be taken
                //if a signal handler is installed, the process will be woken-up
                //,the handler will execute and the process will continue after 
                //pause()
                

     //sigsuspend() suspends(blocks) the current process and also, temporarily
     //replaces the process signal mask with a new mask; if a signal that is
     //not blocked by the temp. mask arrives, the process is woken up and action
     //is taken - the action may be a default or a signal handler may execute and
     //the process may resume after sigsuspend() 
     
     //in our example, we are using another sigsuspend() to block the current process
     //to see the state of the process and the system after several page faults and
     //page frame allocations by this process !!!
     while(1){
     sigdelset(&set1, SIGTERM);
     sigdelset(&set1, SIGINT);
     ret = sigsuspend(&set1);
     }  
     


     return 0; 










}
