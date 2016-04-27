
#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/proc_fs.h>
#include<linux/slab.h>
#include<linux/seq_file.h>
#include<linux/sched.h>
#include<linux/types.h>




static struct proc_dir_entry *entry = NULL ; 

struct task_struct *task; 

static void *
mydrv_seq_start(struct seq_file *seq, loff_t *pos)
{
  
  loff_t off = 0;
 

  for_each_process(task) {
    if (*pos == off++) {
                        printk("in start : success %d\n",(int )*pos);
                        return task;
    }
  }
  printk("in seq_start : over\n");
  return NULL;
}

static void *
mydrv_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
 
	 struct task_struct *taskn = (struct task_struct *)v;   
  ++*pos; 
   printk("in seq_next :%d\n",(int )*pos);
   
if((taskn=next_task(taskn)) != &init_task)
	{
		return taskn;
	}

	
	return NULL;

}


static int
mydrv_seq_show(struct seq_file *seq, void *v)
{
   int ret;
    struct task_struct *tasks = (struct task_struct *)v;

   char *info_str = kmalloc(256,GFP_KERNEL);
  
   printk("in seq_show \n");
   
  
//   sprintf(info_str, "pid is %d tgid is %d stack is %x pname is %s\n", task->pid, task->tgid, task->stack, task->comm);
 sprintf(info_str, "pid is %d tgid is %d stack is %x   taskstate is %lu pname %s \n", tasks->pid, tasks->tgid, tasks->stack, tasks->state,tasks->comm);

   ret = seq_printf(seq,info_str);
   printk(KERN_INFO "the return value of seq_printf is %d\n", ret); 

   kfree(info_str); 
   return 0;

}



static void
mydrv_seq_stop(struct seq_file *seq, void *v)
{

 
   printk("in seq_stop:\n");
}



static struct seq_operations mydrv_seq_ops = {
   .start = mydrv_seq_start,
   .next   = mydrv_seq_next,
   .stop   = mydrv_seq_stop,
   .show   = mydrv_seq_show,
};



static int
mydrv_seq_open(struct inode *inode, struct file *file)
{
  
   printk("we are in mydrv_seq_open\n");   
   

    

  
   return seq_open(file, &mydrv_seq_ops);
}


static struct file_operations mydrv_proc_fops = {
   .owner    = THIS_MODULE,    //this macro will provide the ptr to our module object
   .open     = mydrv_seq_open, /* User supplied */  //passing addresses of functions 
                                                    //to function pointers
   .read     = seq_read,       /* Built-in helper function */
   .llseek   = seq_lseek,       /* Built-in helper function */
   .release  = seq_release,    /* Built-in helper funciton */
};


static int __init
mydrv_init(void)
{
 

  int i;
  
 
  //entry = 
  proc_create("readme", S_IRUSR, NULL,&mydrv_proc_fops);
  if (entry) {
   
   //entry->proc_fops = &mydrv_proc_fops; 
  }
  else 
  {
	//return -EINVAL;
  }
  
  

  printk("we are in init function of the module\n");  //2
  return 0;
}

static void mydrv_exit(void)
{
 
 

   remove_proc_entry("readme", NULL);
   printk("mydrv_exit just executed\n");    

}

module_init(mydrv_init);
module_exit(mydrv_exit);


//add other macros as needed 


