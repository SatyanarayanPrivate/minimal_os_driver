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
     char buf2[512];

     struct my_msgbuf buf1 = { 1, "parent's message"};

     id1 = msgget(KEY1, IPC_CREAT|0600);
     if(id1<0) { perror("msgget"); exit(1);}

    
     ret = fork();  

     if(ret<0) { }

     if(ret == 0) {


	  //id1 = msgget(KEY1, IPC_CREAT|0600);   
	  //error check here also
	  ret1 = msgrcv(id1, &buf1, sizeof(buf1.str), -5,0);
	  if(ret1>0) {

		  printf("the no. of bytes recvd. is %d and string is %s\n",
		          ret1, buf1.str);		  
	  }
	  if(ret1<0) { perror("error in msgrcv"); exit(2);}

     }

     if(ret >0) {


       fgets(buf2,sizeof(buf1), stdin);


	     ret1 = msgsnd(id1,&buf1,strlen(buf1.str)+1, 0);
	     if(ret1 <0) { perror("error in msgsnd"); exit(3);}

     }		     

exit(0);	     
	     
}	  
