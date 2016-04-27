#include<stdio.h>
#include<sys/types.h>
#include<sys/statfs.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
int main()
{

  int ret1,ret2,fd1,fd2;

  struct statfs sfs1;


  ret1 = statfs("/home/test1/testfile1", &sfs1);

  if(ret1<0) { perror("error in statfs call"); exit(1); }

  printf("the type of the file system is 0x%x\n",\
          sfs1.f_type);
  printf("the logical block I/O size is %d\n",\
          sfs1.f_bsize);
  printf("the total no of blocks is  %d\n",\
          sfs1.f_blocks);
  printf("the total no of blocks free is  %d\n",\
          sfs1.f_bfree);
  printf("the total no of blocks available to \
          non super-user is  %d\n",\
          sfs1.f_bavail);
  printf("the total no of inodes is  %d\n",\
          sfs1.f_files);
  printf("the total no of free inodes available is\
           %d\n", sfs1.f_ffree);
  printf("the max. length of a filename is \  
           %d\n", sfs1.f_namelen);
  
  
 exit(0); 

}
