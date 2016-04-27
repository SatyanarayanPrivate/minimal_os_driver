#include<sys/msg.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>

#define KEY1 3333
#define BBUF_SZ 1024
struct payload{
  char file_data[1024]; //absolute pathnames
  char cli_np[1024];    //absolute pathnames
};


//typically, you may launch the server process from one terminal 
//and client process in another terminal - or, you may launch
//server process as a background process and client process as
//a foreground process !!!


struct msgbuf {

 long mtype;   //this must be the type field and must be >0

 struct payload data; // a payload of our size - choice is ours  
};

int main(int argc,char *argv[])
{

   int ret1,ret2,id1,id2,npfdr;
   struct msgbuf msgbuf1;
   char *rdbuf;

   if(argc != 3) { printf("enter <absolue path of filename\
                           >and\
                        absolute path of client fifo name\n");
                   exit(1);
   }

   //KEY is used to uniquely identify the message queue object
   //KEY usage is similar to IP address used to establish 
   //communication between processes over the network !!!
   //IPC_CREAT is used to create an object or use an existing 
   //object
   //0600 flags - multiuser permissions 
   //id is returned and used to access message queue further !!!    

   id1 = msgget(KEY1,IPC_CREAT|0600);

   if(id1<0) { 
      perror("error in msgq creation"); 
      exit(2); 
   }
   //if(id>0) {//will not work }

   //real client code starts
   rdbuf = malloc(BBUF_SZ);
   if(rdbuf==NULL) { printf("error in malloc"); exit(3);}


   msgbuf1.mtype = 1;  
   //we must set the message type of a message in the message buffer
   //to a value >0 - can be 1,2,3,  as long as system supports
   //if we set it to 0, system will return error - we must use
   //a non-zero message type - this is as per rules !!

   //assuming we pass a +ve value for the message type no., 
   //it will stored as part of the message header maintaining
   //this message in the message queue - whenever another process
   //issues a rcv message system call API, the rcv message system 
   //call API will request for a specific message type - if there
   //is a match, system will return the oldest message of the 
   //matching type by scanning the existing set of messages in the
   //message queue !!!

   strncpy(msgbuf1.data.file_data,argv[1],\
           sizeof(msgbuf1.data.file_data));
   ret1 = mkfifo(argv[2],0600); //we are creating a named-pipe file
                                //on the disk - this file is a special file
                                //it does not contain any data-blocks
                                //it has an inode associated with it
                                //it has a directory entry associated with it
                                //it has a different file type - known as
                                //named pipe file type
   if(ret1<0 && errno !=EEXIST)
   {
     perror("error in mkfifo"); 
     exit(4);
   }
   strncpy(msgbuf1.data.cli_np,argv[2],\
           sizeof(msgbuf1.data.cli_np));

   printf("cl1..the operation value is %d\n",msgbuf1.data.op);
   printf("cl1..the filename is %s\n",msgbuf1.data.file_data);
   printf("cl1..the client fifo name is %s\n",\
           msgbuf1.data.cli_np);
   

   //create an instance of message buffer related object 
   //use msgget() with the same KEY as that of receiver
   //in msgsnd() do the following - 
   //id must that of receiver
   //ptr to message buffer related object 
   //first field of the message buffer object 
   //must contain non-zero message type 
   //other fields of the message  buffer object are developer decided
   //third field of the msgsnd uses the size of data stored in the
   //payload 
   //last field of the msgsnd is flags - set it to 0
   //flags may be used, if required  
   //ret here is different from msgrcv() system call API -
   //here, ret just returns success(0) or failure(-1) !!!

   ret1 = msgsnd(id1,&msgbuf1,sizeof(msgbuf1.data),0);
   if(ret1<0) { 
        perror("error in receiving message"); 
        exit(5);
   }

   //npfdr = open(msgbuf1.data.cli_np,O_RDONLY); //this will block until
                                               //another process opens
                                               //this named pipe file
                                               //for write only
   npfdr = open(msgbuf1.data.cli_np,O_WRONLY); //this will block until
                                               //another process opens
                                               //for read  only
   //npfdr = open(msgbuf1.data.cli_np,O_WRONLY); //this will block until
   //npfdr = open(msgbuf1.data.cli_np,O_RDWR); //wrong
   //npfdr = open(msgbuf1.data.cli_np,O_RDONLY|0600); //wrong
   //npfdr = open(msgbuf1.data.cli_np,O_RDONLY,0600); //wrong

   if(npfdr<0) { 
        perror("error in opening named pipe"); 
        exit(6);
   }

   do{
     //if you read from an unnamed pipe or named pipe,
     //read will return 0(EOF) only when there are no
     //write descriptors open for the pipe
     //ret2 = read(npfdr,rdbuf,BBUF_SZ);
     ret2 = write(npfdr,rdbuf,BBUF_SZ)
     if(ret2<0) { perror("error in read"); exit(7); }
     if(ret2==0) break;
     write(STDOUT_FILENO,rdbuf,ret2);
   }while(1);


exit(0);

}
        
           


      


    
    

