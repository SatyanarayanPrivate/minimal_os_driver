#include<errno.h>
#include<asm/unistd.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include "syscallmacros.h"
#include<pthread.h> 
#include<sys/types.h>
void * th1(void*);
void * th2(void*);
extern char **environ;
#define __NR_sys_test_call1 339

_syscall3(ssize_t,write,int,fd,const void *,buf,size_t,count)
_syscall1(long,sys_test_call1,void *,ps_ex )
int main()
{
    
char *str;
   int ret,ret1,p;
pthread_t t1,t2;

struct ps_ex
{
	int stack_size;
	int no_of_threads;
	unsigned int stack_base_addr;
	int uid;
	int curr_sched_policy;
	char file[32];	
}*psx;
	ret=pthread_create(&t1,NULL,th1,NULL);
	ret1=pthread_create(&t2,NULL,th2,NULL);
//int *a,*b;

psx=malloc(sizeof(struct ps_ex));
   ret1 = sys_test_call1(psx);
   if(ret1<0) { perror("error in sys_ps_ex() "); exit(1); }


p=psx->curr_sched_policy;
printf("------------------------PS EXTENDED-----------------------------\n");
printf("\nStack size->\t%u\nStack Base Address:\t%u\n",psx->stack_size,psx->stack_base_addr);
printf("\nNumber of threads:\t%d\n",psx->no_of_threads);
printf("\nUID->\t%d\n",psx->uid);
printf("\nPrgramm->\t%s\n",psx->file);
if(p==0)
printf("\nScheduling Policy:\tNORMAL\n");
if(p==1)
printf("\nScheduling Policy:\tFIFO\n");
if(p==2)
printf("\nScheduling Policy:\tRR\n");
if(p==3)
printf("\nScheduling Policy:\tBATCH\n");






//printf("\n%d ",psx->stack_size);
//printf("\n%u\t uid=%d\npolicy=%d\n,\t%d=num of threads\n",psx->stack_base_addr,psx->uid,psx->curr_sched_policy,psx->no_of_threads);
pthread_join(t1,NULL);
pthread_join(t2,NULL);
exit(0); 
}
              

void*  th1(void * a)
{
	sleep(2);
}
void* th2(void *b)
{
	sleep(3);
}
