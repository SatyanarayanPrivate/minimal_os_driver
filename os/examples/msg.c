#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>

#define KEY1 1234

struct my_msgbuf{

    long mtype;
    char str[256];
};    

int main(int argc, char *argv[])
{

     int id1,id2,ret,ret1;

     struct my_msgbuf buf1 = { 1, "parent's message"};
     struct my_msgbuf buf2 = { 2, "child's acknowledgement"};

     id1 = msgget(KEY1, IPC_CREAT|0600);
     if(id1<0) { perror("msgget"); exit(1);}

    
     ret = fork();  


     if(ret<0) { }//add error handling code 

     if(ret == 0) {


	  //id1 = msgget(KEY1, IPC_CREAT|0600);   
	  //error check here also
	  ret1 = msgrcv(id1, &buf1, sizeof(buf1.str), 1,0);
	  if(ret1>0) {

		  printf("the no. of bytes recvd. is %d and string is %s\n",
		          ret1, buf1.str);

                  //send ???		  
	  }
	  if(ret1<0) { perror("error in msgrcv"); exit(2);}

     }

     if(ret >0) {

	     ret1 = msgsnd(id1,&buf1,strlen(buf1.str)+1, 0);
	     if(ret1 <0) { perror("error in msgsnd"); exit(3);}
                   
	     ret1 = msgrcv(id1, &buf2, sizeof(buf2.str), 2,0);
     }		     

exit(0);	     
	     
}	  
