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

   fd = open(argv[1], O_RDONLY);

   if(fd<0) { perror("error in opening"); exit(1); }

   ret = fstat(fd,&sb);

   if(ret<0) { perror("error in fstat"); exit(2); }

   region = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd,0);
    
   if(region == (char *)-1){ perror("error in mmap"); exit(3); }

   close(fd);

   ret = write(STDOUT_FILENO, region, sb.st_size); 
  
   if(ret<0) { perror("error in writing to the standard o/p"); exit(4);}

   ret =  munmap(region,sb.st_size);
  
   if(ret<0) { perror("error in unmapping"); exit(4); }


   *(char *)region = 'a'; 

   exit(0);   
}
    
