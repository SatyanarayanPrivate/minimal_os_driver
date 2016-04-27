#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>
int main()
{

  int ret1,ret2,fd1,fd2;
  struct stat st1,st2;
  char buff[2048],target_name[256];

  //pause();   //it is a system call which will wait for a signal
             //and if a signal arrives, it will unblock and 
             //the process will continue 
  
  //*(char *)0x00000000 = 'a'; 
  //*(char *)0x00000080 = 'a'; 
  //*(char *)0x0804afff  = 'a'; 

  //fd1 = open(argv[1], O_RDONLY);

  fd1 = open("/home/test1/testfile1", O_RDONLY);
  if(fd1<0) { perror("error in opening"); exit(1); }

  //opening a device file
  //fd1=open("/dev/plp_kmem0,O_RDONLY); 
  //fd1=open("/dev/plp_kmem0,O_RDONLY|O_NONBLOCK); 
  // 
  //ret = read(fd1,buff,LEN); //read from the open file
  //ret = write(fd1,buff,LEN); //write to the open file


  fd1 = open("/home/test1/testfile1", O_RDONLY|O_CREAT,0600);
  fd1 = open(argv[2], O_RDWR|O_CREAT,0600);
  


  fd1 = open("/home/test1/testfile1", O_WRONLY|O_APPEND|O_TRUNC);
  //fd1 = open("/home/test1/testfile1", O_WRONLY|O_TRUNC);
  //fd1 = open("/home/test1/testfile1", O_WRONLY|O_APPEND);
  //fd1 = open("/home/test1/testfile1", O_APPEND);//wrong - missing O_WRONLY
  //fd1 = open("/home/test1/testfile1", O_RDWR|O_CREAT,0664);
  if(fd1<0)  { perror("error in open"); exit(1); }
  close(fd1);

  ret1 = link("/home/test1/testfile1", "/home/test1/testfile1_hl");
  if(ret1<0 && errno!= EEXIST) { perror("error in link"); exit(2); }
  
  ret1 = symlink("/home/test1/testfile1", \
                 "/home/test1/testfile1_sl");
  if(ret1<0 && errno != EEXIST) { 
     perror("error in symlink"); exit(3); }
 
  ret2 = stat("/home/test1/testfile1", &st1);
  if(ret2<0) { perror("1..error in stat"); exit(4); }

  printf("1..the device-id is %d\n",st1.st_dev); 
  printf("1..the inode no is %d\n",st1.st_ino); 
  printf("1..the no. of hard-links is %d\n",st1.st_nlink); 
  printf("1..the uid is %d\n",st1.st_uid); 
  printf("1..the gid is %d\n",st1.st_gid); 
  printf("1..the size is %d\n",st1.st_size); 
  printf("1..the logical block-size is %d\n",st1.st_blksize); 
  printf("1..the no.of blocks allocated is %d\n",\
          st1.st_blocks/st1.st_blksize);  


  
  ret2 = stat("/home/test1/testfile1_hl", &st1);
  if(ret2<0) { perror("1..error in stat"); exit(5); }

  printf("2..the device-id is %d\n",st1.st_dev); 
  printf("2..the inode no is %d\n",st1.st_ino); 
  printf("2..the no. of hard-links is %d\n",st1.st_nlink); 
  printf("2..the uid is %d\n",st1.st_uid); 
  printf("2..the gid is %d\n",st1.st_gid); 
  printf("2..the size is %d\n",st1.st_size); 
  printf("2..the logical block-size is %d\n",st1.st_blksize); 
  printf("2..the no.of blocks allocated is %d\n",\
          (st1.st_blocks*512)/st1.st_blksize);
        
  ret2 = write(fd1,"this is a test message",22);   
  ret2 = stat("/home/test1/testfile1", &st1);
  if(ret2<0) { perror("1..error in stat"); exit(4); }

  printf("3..the size is %d\n",st1.st_size); 
  printf("3..the logical block-size is %d\n",st1.st_blksize); 
  printf("3..the no.of blocks allocated is %d\n",\
          (st1.st_blocks*512)/st1.st_blksize);

  readlink("/home/test1/testfile1_sl",target_name,\
           sizeof(target_name));
  printf("the target name is %s\n", target_name);

  ret2 = stat("/home/test1/testfile1_sl", &st1);

  ret2 = lstat("/home/test1/testfile1_sl", &st1);
  if(ret2<0) { perror("1..error in stat"); exit(4); }
  printf("4..the size is %d\n",st1.st_size); 
  printf("4..the logical block-size is %d\n",st1.st_blksize); 
  printf("4..the no.of blocks allocated is %d\n",\
          (st1.st_blocks*512)/st1.st_blksize);

  close(fd1);
  //exit() is a library call - it in turn, calls _exit() system call
  //please find what is done in addition to _exit() by exit() library call
  //hint - stdio library calls

  unlink("/home/test/file1"); //deletes a hard-link 
  exit(0);

}
