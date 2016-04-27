#include<stdio.h>
#include<sys/types.h>
#include<sys/statvfs.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
int main()
{

  int ret1,ret2,fd1,fd2;

  struct statvfs sfs1;


  ret1 = statvfs("/home/test1/testfile1", &sfs1);

  if(ret1<0) { perror("error in statfs call"); exit(1); }

  printf("the logical block I/O size is %d\n",\
          sfs1.f_bsize);
  printf("the fragment size is  %d\n",\
          sfs1.f_frsize);
  printf("the total no of blocks free is  %d\n",\
          sfs1.f_bfree);
  printf("the total no of blocks available to \
          non super-user is  %d\n",\
          sfs1.f_bavail);
  printf("the total no of inodes is  %d\n",\
          sfs1.f_files);
  printf("the total no of free inodes available is\
           %d\n", sfs1.f_ffree);
  printf("the total no of free inodes available to non-root is\
           %d\n", sfs1.f_favail);
  printf("the max. length of a filename is \  
           %d\n", sfs1.f_namemax);
  
  
 exit(0); 

}
