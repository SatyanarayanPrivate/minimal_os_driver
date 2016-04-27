#include<errno.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main(int argc, char **argv)
{

   int fd,ret;
   struct stat sb;

   char  * region;

   fd = open(argv[1], O_RDWR);

   if(fd<0) { perror("error in opening"); exit(1); }

   ret = fstat(fd,&sb);

   if(ret<0) { perror("error in fstat"); exit(2); }

   //a given file is mapped to virtual address space of
   //the current process 
   //once mapped, a process can access the contents of the
   //file using virtual addresses, not file system call  APIs !!!

   region = mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE, 
                 MAP_SHARED, fd,0);
   
   printf("the size is %d\n", sb.st_size); 
   if(region == (char *)-1){ perror("error in mmap"); exit(3); }

   close(fd);

   ret = write(STDOUT_FILENO, region, 8*sb.st_size);

    
   if(ret<0) { perror("error in writing to the standard o/p"); exit(4);}
   *(char*)(region+8192)  = 'a';

   exit(0);   
}
    
