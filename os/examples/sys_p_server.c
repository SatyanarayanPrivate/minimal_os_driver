#include<sys/msg.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>
#include<errno.h>

#define KEY1 3333

#define BBUF_SZ 1024

struct payload{
  char op;               //unused field
  char file_data[256];   //char message1[256];
  char cli_npipe[256];   //char message[256];
};

struct msgbuf {

 long mtype; 

 struct payload data;
};



//try to understand the below handler and use the same, when 
//coding signal related assignments !!

void sig_chld(int signo)
{

  int ret1;

  //test the handler with the while loop and without !!
  //find what is the reason for adding the while loop !!!

  while(1){
      //Note : signal handlers are not supposed to block
      //
      //waitpid() is a blocking system call API, by default 
      //since we are using it in signal handler, we cannot
      //use it as it is !!! we must use a special flag 
      //that will ensure that waitpid() will never block 
      //assuming we do not use flags, and use 0 for the
      //last parameter, waitpid() will block current 
      //process context when no child of this process
      //is currently in zombie state !!! 
      //if we use WNOHANG, this is overcome - with WNOHANG,
      //if there are no children of this priocess 
      //in zombie state, waitpid() will return 0, does 
      //not block !!! this is the reason for using 
      //WNOHANG !!! 
      //if WNOHANG is omitted, whenever waitpid() is executed
      //and currently no child process is in terminated (some other state)
      //, waitpid() will block the parent process until one of the
      //children terminate and enter zombie state 


      //WNOHANG flag is added such that 
      //waitpid() will not block, if there are children and
      //none are in terminated(zombie) state - it will return 0
      //summary - if waitpid() is called with WNOHANG, ret values are:
      //+ve - if a child is cleaned-up
      //0   - if there are children and none are in zombie state
      //-1  - if there are no children in any state or there is an error
      //if you need more details, dig deep into the man pages !!!
      ret1 = waitpid(-1,&status,WNOHANG);
      if(ret1<0) break;
      if(ret1==0) break;
      if(ret>0) { //keep count of the no. of children terminated and
                  //cleaned up - this is maintained in counter
                  //this counter can be used along with sigsuspend() 
                }

  }

}      


int main()
{


   int ret,ret1,ret2,ret3,id1,id2,fd,npfd;
   struct msgbuf msgbuf1;
   struct payload pld1;
   struct sigaction act1,act2;
   sigset_t set1,set2;

   char *rdbuf;

   //install signal handler for SIGCHLD using sigaction

   sigfillset(&set1);                 //library call
   sigdelset(&set1,SIGCHLD);          //library call
   sigprocmask(SIG_SETMASK,&set1,&set2);//system call //mask is used outside signal 
                                                     //handlers

   //what will be the action/delivery/processing done by the 
   //system, when a child process terminates and generates 
   //a SIGCHLD signal to the parent process - let us assume
   //that the current signal handling for SIGCHLD in the 
   //parent is default action !!! meaning, no signal handler 
   //is installed by in the parent process code !!!
   //
   //as per standard rules of signals/signal handling, parent
   //process may be terminated, but it is not true in this case -
   //meaning, in the case of SIGCHLD, if the signal is generated
   //and no signal handler is installed, system ignores such 
   //SIGCHLD signals and no action is taken - meaning, default 
   //action for SIGCHLD is ignoring the signal - however, 
   //this is not true in the case of all signals - say, SIGTERM,
   //SIGINT have a default action of termination - refer to 
   //process_unix.txt under day3_4_5/ and also man 7 signal 
   //for other signals 

   //act1.sa_handler = SIG_DFL; //install the default action 
   //act1.sa_handler = SIG_IGN; //action will be to ignore 
   act1.sa_handler = sig_chld; //install a signal handler
   act1.sa_flags = 0;
   sigfillset(&act1.sa_mask);//mask used during the signal handler's execution only
   //you must call sigaction() for each signal that be specially 
   //handled by the system/process

   //after this sigaction() API is executed, whenever a child process
   //terminates and SIGCHLD signal is generated for the corresponding 
   // parent, our installed signal handler will be invoked, when SIGCHLD
   //signal is delivered/processed by the system  
   sigaction(SIGCHLD,&act1,&act2);
   //sigaction(SIGINT,&act1,NULL);

   rdbuf = malloc(1024);
   if(!rdbuf) { 
     printf("error in malloc");
     exit(1);
   }

   ret = mkfifo(argv[1],0600);//create server fifo if needed 
   if(ret<0 && errno != EEXIST) { perror(""); exit(2); }

  //a server process waits for requests using a IPC mechanism 
  //whenever a new request arrives, a new child process is created
  //and request is passed to the child process to be processed
  //parent process after doing the above will once again block
  //on the IPC mechanism waiting for requests 

  //it is the responsibility of the child process to service 
  //a given request and terminate 
  //when a child process terminates, it enters zombie state
  //it is the responsibility of the parent process to 
  //clean up the child process !!! however, the parent process
  //is blocked and waiting for requests - this means, the children
  ///processes will terminate and enter zombies states, but not 
  //cleaned up !!!


  //if the parent process installs a signal handler for SIGCHLD 
  //signal, whenever a child process is terminated, system will  
  //implicitly generate SIGCHLD signal to the parent process !!!
  //it is the responsibility of the system to execute the 
  //corresponding signal handler on behalf of the parent process
  //in addition, to so, it is the responsibility of the system to
  //wake up the parent process when a SIGCHLD is generated as
  //a consequence of a terminated child process !!!

  //in this scenario, the most common case is that the parent 
  //process will be blocked and waiting for requests - which means,
  //system has to first wake up the parent process and then,
  //signal handling must be managed !!!

  //in this case, we still say that SIGCHLD is an asynchronous
  //signal and its handling will also  be asynchronous with respect 
  //to the main() of the parent process !!!



   while(1){ //server continues to serve
   npsfd = open(argv[1], O_RDONLY);   
   if(npsfd<0) { 
       temp_errno = errno; perror(""); if(temp_errno == EINTR) continue;  
       exit(3); }

   //let us assume that the parent process is currently blocked
   //in the read() system call API - what will happen 
   //if SIGCHLD is generated by a terminating child process ???

   // as we saw in the example1.c, if a process is blocked in 
   //a system call and a signal arrives, process is woken up - 
   //due to the implementation, process will be resumed in user-space,
   //but signal handler will be invoked(this is actually a jump)

   //once the signal handler is completed, using certain tricks ,
   //system will again jump back to system space and 
   //return back to the system call API, as expected - in this context,
   //read() is the system call API and sys_read() is the internal 
   //system call system routine !!!

   //this return from read() will not be a normal return - it 
   //is a premature wake up and the return value will be -1 and
   //errno value of the current process is set to EINTR !!!
   ret = read(npsfd,&pld1,sizeof(pld1));

   if(ret<0 && errno==EINTR) { continue;  }
   if(ret<0) { perror(""); exit(4); }
   
   if(ret==0) continue;  //end of file case is handled without redundant children
   //message has come
   close(npsfd);
   ret = fork(); //create a child per request

   if(ret<0) { perror(""); exit(5); }
   if(ret>0) { close(npsfd); continue; }

   if(ret==0) {
        close(npsfd);  //closing the server name pipe 
	ffd = open(pld1.file_data,O_RDONLY); //opening the data-file
        //add error checking
        npcfd = open(pld1.cli_npipe,O_WRONLY); //opening the npipe of client
        while( (ret=read(ffd,buf,sizeof(buf))) > 0){
                write(npcfd,buf,ret);
        }
        exit(0);
    }	 
  } //while loop

   id1 = msgget(KEY1,IPC_CREAT|0600);

   if(id1<0) { 
      perror("error in msgq creation"); 
      exit(2); 
   }

   while(1)
   {
       //waitpid(-1,&status,0);     //wrong place -- 1
  
       //server waits for message from clients
       //
       ret1 = msgrcv(id1,&msgbuf1,sizeof(msgbuf1.data),0,0);
       
       if(ret1<0 && errno == EINTR) continue;
       if(ret1<0) { 
          perror("error in receiving message"); 
          exit(3);
       }

       //fork and let the child handle the client
       ret1 = fork();
       if(ret1<0) { perror("error in fork"); exit(4); }
       if(ret1>0) {

                //waitpid(-1,&status,0);   //wrong place -- 2
                continue;
       }
       if(ret1==0)
       {
             printf("S..the operation value is %d\n",\
                     msgbuf1.data.op);
       	     printf("S..the filename is %s\n",\
                     msgbuf1.data.file_data);
             printf("S..the client fifo name is %s\n",\
                     msgbuf1.data.cli_np);

             fd = open(msgbuf1.data.file_data,O_RDONLY);
             if(fd<0) { perror("error in opening data file");
                         exit(4);
             }
             npfd = open(msgbuf1.data.cli_np,O_WRONLY);
             if(npfd<0) { 
                  perror("error in opening named\
                          pipe");
                  exit(5);
             }

             printf("S...just after file is open and \
                     fifo is open\n");

             if(msgbuf1.data.op){
       
             do{

               ret2 = read(fd,rdbuf,BBUF_SZ);//reading from a reg. file
               if(ret2<0) { 
               perror("error in reading from datafile");       
               exit(6); //can be somethingelse
               }
               if(ret2 == 0) break;
               //if you write to the pipe and no read descriptors to
               //the pipe are open, the system will generate SIGPIPE
               //signal to the process that is writing 
               ret3 = write(npfd,rdbuf,ret2); //writing to a FIFO
             } while(1);

              printf("S...just after writing the file data\n");
              close(npfd);
             }//if data.op==1
             exit(0);       

          } //child ends here
    }// the server while

exit(0);

}
        
           


      


    
    

