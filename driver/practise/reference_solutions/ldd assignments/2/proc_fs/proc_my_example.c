#include<linux/init.h>
#include<linux/list.h>
#include<linux/sched.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/proc_fs.h>
#include<linux/slab.h>
#include<linux/seq_file.h>
//fp is file pointer and dir is the pointer to the directory in proc fs

static struct proc_dir_entry *fp = NULL ,*dir=NULL; 

/*start mehod----------------------------------------------------------------------*/

static void * mydrv_seq_start(struct seq_file *seq , loff_t *pos)
	{

		struct task_struct *p;//=kmalloc(sizeof(struct task_struct ,GFP_KERNEL));
	        loff_t off=0;
		dump_stack(); // just to track the process 

		for_each_process(p) 
			{
			   if (*pos == off++) 
			   	{
				      printk("in start : success %x\n",*pos);
				     //  if(off>50)
				    //	       return NULL ;
				   //     else
					       return p;
				}
			 }
		    printk("in seq_start : over\n");
		    return NULL;
			
	}
/*next method-----------------------------------------------------------------------*/
/*here the main task is to increment the pos value*/
static void * mydrv_seq_next(struct seq_file *seq , void *v ,loff_t *pos)
	{
		struct list_head *n =((struct task_struct *)v)->tasks.next;
		++*pos; //advancing yhe pos value
		printk("in seq_next :%x\n",*pos);
		  
		  return (n != &init_task.tasks) ? list_entry(n,struct task_struct,tasks) : NULL;
	   
	}
/*show method------------------------------------------------------------------------*/
/*here load the data (pid and ppid and tgid) in to file test*/
static int mydrv_seq_show(struct seq_file *seq, void *v)
{
	   int ret;
	   const struct task_struct *p =v;
           char *info_str = kmalloc(256,GFP_KERNEL);  /* Interpret the iterator, 'v' */
           printk("in seq_show \n");
		       
                             // what ever information that you wish to generate on the fly, it 
		             // must be generated and passed as below 
		             // using seq_printf() we can pass info. to user space 
		             // collect information from pd and pass it to system space
		             // buffer using seq_printf
	        sprintf(info_str, "pid is %d tgid is %d threadsize = %d\n", p->pid, p->tgid,THREAD_SIZE);		       
                ret = seq_printf(seq,info_str);
	        printk(KERN_INFO "the return value of seq_printf is %x\n", ret); 		       
	        kfree(info_str); 
  return 0;
}
/*stop method------------------------------------------------------------------------------------*/
static void
mydrv_seq_stop(struct seq_file *seq, void *v)
{

	 //  stop() is normally used for freeing any resource allocated during
	      //start() or show()
	         /* No cleanup needed in this example */
	            printk("in seq_stop:\n");
		    seq_printf(seq,"END\n");
}	  
/*sequence  operations-----------------------------------------------------------------------------------*/
static struct seq_operations mydrv_seq_ops = {
						   .start = mydrv_seq_start,
						   .next   = mydrv_seq_next,
						   .stop   = mydrv_seq_stop,
						   .show   = mydrv_seq_show,
						};
/*opening our test file */
static int
mydrv_seq_open(struct inode *inode, struct file *file)
{
	  // printk writes the message in the kernel's log-buffer 
	  //using dmesg, you can read from the kernel's log-buffer 
	  printk("we are in mydrv_seq_open\n");   //1
	  /* Register the operations  */
	  dump_stack();  //used for diagnostic messages  
	  return seq_open(file, &mydrv_seq_ops);
}
/*file operations ------------------------------------------------------------------------------------------*/	   
static struct file_operations mydrv_proc_fops = {
	                        		.owner    = THIS_MODULE,    //this macro will provide the ptr to our module object
	              	                        .open     = mydrv_seq_open, /* User supplied   //passing addresses of functions 
	                                                                                to function pointers*/
	                                        .read     = seq_read,       /* Built-in helper function */
	                                        .llseek   = seq_lseek,       /* Built-in helper function */
	                                        .release  = seq_release,    /* Built-in helper funciton */
	                                        };
	   	  	
/*--init method (process) here we have to create dir and file into that dir------------------------- */		
static int __init mydrv_init(void)
{

	
          dir=proc_mkdir("proc_test",NULL); // creating a dir of name proc_test 
          fp = create_proc_entry("test", S_IRUSR, dir); // creating a file in proc_test dir of name test
	  if(fp)
	        {
		   fp->proc_fops=&mydrv_proc_fops;
		 }
           else
		{	
			return -EINVAL;	
		}
   	 	   
 	  printk("we are in init function of the module\n"); 
	  return 0;
}
/* exit method here we have to free the pointers which were dynamically allocated some memory--------------*/
static void mydrv_exit(void)
{
  

   remove_proc_entry("test",dir); //removing the file in the proc_test directory
   remove_proc_entry("proc_test", NULL); //removing the proc_test directory
   printk("mydrv_exit just executed\n");  

}

module_init(mydrv_init); 
module_exit(mydrv_exit);

MODULE_LICENSE("GPL");


