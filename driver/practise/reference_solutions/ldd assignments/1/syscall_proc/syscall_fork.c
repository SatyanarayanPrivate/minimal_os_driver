#include<pthread.h>
#include<errno.h>
#include<asm/unistd.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include "syscallmacros.h"   //this header includes the wrappers 
                             //we can use appropriate wrappers 

struct custominfo
{
	unsigned int uid;
	unsigned int sched_policy;
	unsigned int stack_addr;
	unsigned int stack_size;
	unsigned int no_threads;
};

extern char **environ;

#define __NR_test_call 338 //338
#define __NR_testcall 339  //339

//_syscall3(ssize_t,write,int,fd,const void *,buf,size_t,count)

//_syscall3(long,open,const char *,filename,int,flags,int,mode)

//_syscall0(pid_t,getpid)
//_syscall0(pid_t,fork)

_syscall2(long,test_call,unsigned int*,pid,unsigned int *,tgid)
_syscall1(long,testcall,struct custominfo *, custom)
//_syscall1(long,testcall,unsigned int*,uid)
int main()
{
    

   int ret,ret1,ret2;
   unsigned int pid,tgid;
   struct custominfo custom;

   char str[1024];

/*   ret = snprintf(str,sizeof(str), "the pid of the process is %d\n", getpid());
  
   ret1 = write(STDOUT_FILENO,str,ret+1);

   if(ret1<0){ perror("error in write syscall"); exit(1);}

   ret1 = getpid();
   

   if(ret1<0){ perror("error in open syscall"); exit(2);}*/

  ret1 = test_call(&pid,&tgid);
   	 if(ret1<0) 
		{ 
			perror("error in test_syscall"); 
			exit(1);
		 }
 		 printf("test_syscall returned pid %d and tgid %d\n",pid,tgid);
   
  ret2 = testcall(&custom);
   	if(ret2<0) 
   		{
		   perror("error in testcall\n"); 
		   exit(2);
		}

	 printf("testcall returned\n uid =%d\n scheduling policy= %d\n %d  stack size in system space \n",custom.uid,custom.stack_addr,custom.sched_policy,custom.stack_size);

   exit(0); 
}
              


