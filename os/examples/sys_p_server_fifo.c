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
  char file_data[1024];   //char message1[256];
  char cli_npipe[1024];   //char message[256];
};

struct msgbuf {

 long mtype; 

 struct payload data;
};

void sig_chld(int signo)
{

  int ret1;

  while(1){
      //Note : signal handlers are not supposed to block
      //WNOHANG flag is added such that 
      //waitpid() will not block, if there are children and
      //none are in terminated(zombie) state - it will return 0
      //summary - if waitpid() is called with WNOHANG, ret values are:
      //+ve - if a child is cleaned-up
      //0   - if there are children and none are in zombie state
      //-1  - if there are no children in any state or there is an error
      ret1 = waitpid(-1,NULL,WNOHANG);
      if(ret1<0) break;
      if(ret1==0) break;

  }

}      


int main(int argc,char *argv[] )
{


   int temp_errno,ret,ret1,ret2,ret3,id1,id2,fd,ffd,npsfd,npcfd;
   struct msgbuf msgbuf1;
   struct payload pld1;
   struct sigaction act1,act2;
   sigset_t set1,set2;

   char *buf;

   buf = malloc(2048);

   ret = mkfifo(argv[1],0600);//create server fifo if needed 
   if(ret<0 && errno != EEXIST) { perror(""); exit(2); }
   while(1){ //server continues to serve
   npsfd = open(argv[1], O_RDONLY);   
   if(npsfd<0) { 
       temp_errno = errno; perror(""); if(temp_errno == EINTR) continue;  exit(2); }

   ret = read(npsfd,&pld1,sizeof(pld1));

   if(ret<0) { perror(""); exit(3); }
   
   if(ret==0) continue;  //end of file case is handled without redundant children
   //message has come
   close(npsfd);
   ret = fork(); //create a child per request

   if(ret<0) { perror(""); exit(4); }
   if(ret>0) { close(npsfd); continue; }

   if(ret==0) {
        //mask all signals 

        close(npsfd);  //closing the server name pipe 
	ffd = open(pld1.file_data,O_RDONLY); //opening the data-file
        //add error checking
        npcfd = open(pld1.cli_npipe,O_WRONLY); //opening the npipe of client
        while( (ret=read(ffd,buf,sizeof(buf))) > 0){
                write(npcfd,buf,ret);

        //reenable all signals 
        }
        exit(0);
    }	 
  } //while loop

}
        
           


      


    
    

