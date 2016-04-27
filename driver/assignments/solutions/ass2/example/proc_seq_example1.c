//include appropriate headers
//
//

#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/proc_fs.h>
#include<linux/slab.h>
#include<linux/seq_file.h>


//refer to ch11 of LDD/3
/* Private Data structure */
struct _mydrv_struct {
   /* ... */
   struct list_head list; /* Link to the next node */  //this is fixed for any type of struct
   char info[12];         /* Info to pass via the procfs file */ //these elements differ
                          //based on your module/driver
   /* ... */
};

//refer to ch11 of LDD/3
static LIST_HEAD(mydrv_list);  /* List Head */
static struct proc_dir_entry *entry = NULL ; 


/* start() method */
//
//whatever index is requested, we send back the pointer of that object
//
//pos gives the starting element in your list 
//typically, it is 0
//*pos tells us where are we in the list access - 
//                                        the specific node in the list  

//param1 is the object created for this active file in the 
//sequence file layer - it may be used in our operations ' code 
//
//param2 is pointer to the offset field in the open file object of 
//this active file - however, in this context it does not represent 
//file logical byte no - instead, it represents index of the system 
//object , in the system's data structure that is navigated, when 
//this procfs file is accessed using an active file !!! in fact, when 
//an active file of this procfs file is accessed/navigated/read, 
//data from system objects maintained in the corresponding system 
//data structure is retrieved by passed to user-space !!!
//
//system passes a *pos (offset/index of an object), as per the
//current state of navigation/access !!! to start with the value will
//be 0 - as one or more objects are navigated/accessed, this value will
//be incremented - we must interpret this value, validate the value and
//take appropriate actions, as per rules !!!
//
//as per rules, we must do the following:
//- using the *pos(index value), navigate and extract ptr to
//  object 
//- return ptr to the extracted object
//- otherwise return NULL 
//- all we need to code is the above rules and implement the same !!!
static void *
mydrv_seq_start(struct seq_file *seq, loff_t *pos)
{
  struct _mydrv_struct *p;
  loff_t off = 0;
  /* The iterator at the /requested offset */
 
//  dump_stack(); 

  list_for_each_entry(p, &mydrv_list, list) {
    if (*pos == off++) {
                        printk("in start : success %l\n",*pos);
                        return p;
    }
  }
  printk("in seq_start : over\n");
  return NULL;
}

/* next() method */
//
//advance to the next position and return the pointer to that object
//

//as per the rules, we must do the following in this method :
//
//using ptr to the current object, get the ptr to the next object 
//in the system data-structure !!!
//
//in addition, increment the offset (index) using *pos !!!
//
//eventually, return ptr to the next object in the data structure

//however, if we have navigated the list completely, return NULL !!!
//





static void *
mydrv_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
  //here, our real objective is to increment the *pos 
  //and return ptr to the next object based on what is 
  //passed to us by system 

  //we still must check if the next element is the first element
  //and if so , we have reached end of list and we must 
  //return NULL  

   //dump_stack();

  /* 'v' is a pointer to the iterator returned by start() or
     by the previous invocation of next() */
  struct list_head *n = ((struct _mydrv_struct *)v)->list.next;
  ++*pos; /* Advance position */
  
   /* Return the next iterator, which is the next node in the list */
   printk("in seq_next :%l\n",*pos);
   


  return(n != &mydrv_list) ?
          list_entry(n, struct _mydrv_struct, list) : NULL;
}


/* show() method */
//invariably, show() method is invoked just after start() method !!!
//in addition, show() method is passed the ptr to the object that 
//was returned by start() to seq_read() !!!
//
//in addition, we must copy data from the object pointed to by 
//v into the buffer maintained in struct seq_file object pointed to 
//by seq - there are helper functions to do this - do not do by 
//hand - if you do so, it will corrupt any existing data in the
//buffer !!!
//these actions are taken as per rules - if you need more information, 
//refer to document that contains the rules !!!

static int
mydrv_seq_show(struct seq_file *seq, void *v)
{
   int ret;
   struct _mydrv_struct  *p =v;

   //dump_stack();
   /* Interpret the iterator, 'v' */
   printk("in seq_show \n");
   
   //what ever information that you wish to generate on the fly, it 
   //must be generated and passed as below

   //using seq_printf() we can pass info. to user space 
   //

   //buffer using seq_printf(provided by sequence file layer)
   //we are copying data from a current object of navigation into
   //buffer of *seq !!!
   ret = seq_printf(seq,"%s\n",p->info);
   printk(KERN_INFO "the return value of seq_printf is %d\n", ret); 

   return 0;
}


/* stop() method */
static void
mydrv_seq_stop(struct seq_file *seq, void *v)
{

   //stop() is normally used for freeing any resource allocated during
   //start() or show()
   /* No cleanup needed in this example */
   //dump_stack();
   printk("in seq_stop:\n");
}



//if you have understood the rules for coding the below methods,
//you can code and provide you set of methods to the sequence file
//layer, which inturn serves the procfs layer 
//
//whenever, our procfs file is accessed, associated data structure
//is navigated and information extracted from objects maintained
//in the data structure  are copied to users-space buffer - however, 
//the whole frame-work works, if you have written the methods as per 
//rules and validated their working !!!

/* Define iterator operations */
static struct seq_operations mydrv_seq_ops = {
   .start = mydrv_seq_start,
   .next   = mydrv_seq_next,
   .stop   = mydrv_seq_stop,
   .show   = mydrv_seq_show,
};

//this method is invoked, when active file is created, when 
//our pseudofs file is opened using open(" ", )

static int
mydrv_seq_open(struct inode *inode, struct file *file)
{
   //printk writes the message in the kernel's log-buffer 
   //using dmesg, you can read from the kernel's log-buffer 
   printk("we are in mydrv_seq_open\n");   //1
   /* Register the operations */

    

//   dump_stack();  //used for diagnostic messages

   //with the help of dump_stack(), we understand that our
   //open method is invoked by procfs layer, which is connected
   //to logical file system manager 

   //further, our open method also registers another table
   //of methods/operations using seq_open() !!! this system API
   //enables us to register another set of operations with 
   //sequence file layer of the system !!! this must be done 
   //as per the rules of pseudo files of procfs !!!
  
   //as per the rules and what is available from 
   //<ksrc>/include/linux/seq_file.h & <ksrc>/fs/seq_file.c, 
   //following is done during seq_open()
   //
   //internally, struct seq_file {} object is created and 
   //filled with another operations table ptr and in addition, 
   //the new struct seq_file {} object is linked to active file
   //object of the current active file !!!this is all done with 
   //respect to sequence file layer, not procfs layer - however,
   //procfs and sequence file layer are connected !!!

   return seq_open(file, &mydrv_seq_ops);
}

//for the current context, a file_operations object is created
//and initialized as per rules of the system - these rules are
//documented under <ksrc>/Documentation/filesystems/seq_file.txt 
//
//every kernel module object file creates a struct module{} in the 
//system space, when loaded - this struct module{} is used to manage
//the loaded kernel module object file, in the system !!!

static struct file_operations mydrv_proc_fops = {
   .owner    = THIS_MODULE,    //this macro will provide the ptr to our module object
   .open     = mydrv_seq_open, /* User supplied */  //passing addresses of functions 
                                                    //to function pointers
   .read     = seq_read,          //Built-in helper functions taken from 
                                  //fs/seq_file.c 
   .llseek   = seq_lseek,         //fs/seq_file.c provides sequence file
                                  //layer - sequence file layer provides
   .release  = seq_release,       //support to procfs layer 
};


static int __init
mydrv_init(void)
{
   /* ... */

  int i;
  struct _mydrv_struct *mydrv_new;
  /* ... */
  /* Create /proc/readme */
  //this will create a new proc file with following attributes !!!
  //name 
  //permissions
  //ptr to the parent directory's object (proc_dir_entry{})
  //last parameter may be NULL, if our file is to be created
  //under /proc directory as the parent directory - otherwise,
  //it must not be NULL !! 
  //return value provides pointer to struct proc_dir_entry{} of the
  //new proc file that has been created !! a newly created procfile 
  //object is added to a list under the object of the corresponding 
  //parent directory !!!
  //refer to include/linux/proc_fs.h 

  //refer to fs/proc/generic.c
  entry = create_proc_entry("readme", S_IRUSR, NULL);//a file is created 
  /* Attach it to readme_proc() */
  //check for error - NULL 
  if (entry) {
   /* Replace the assignment to entry->read_proc in proc_example1.c
      with a more fundamental assignment to entry->proc_fops. So
      instead of doing "entry->read_proc = readme_proc;", do the
      following: */

      //we are replacing an entry in the proc_dir_entry to our convenience
   //this is a form of registration, where a table of our methods
   //are added to the system via a system object's field - in this case,
   //we are using proc_fops field of proc_dir_entry {} - this is done
   //as per the rules !!! by doing this, we are providing custom 
   //operations to our procfs file - whenever, our procfs file is 
   //accessed, these custom methods will be accessed and accordingly,
   //data will be processed/retrieved !!!
   entry->proc_fops = &mydrv_proc_fops; 
  }
  else 
  {
	return -EINVAL;
  }
  
  /* Handcraft mydrv_list for testing purpose.
     In the real world, device driver logic or kernel's  sub-systems 
     maintains the list and populates the 'info' field */
    //we are creating our own objects and maintaining them in a 
    //system defined data-structure - all rules/apis are provided
    //by the system !!!
    //refer to chapter 11 of LDD/3 for details of objects and 
    //data structure !!!
    //kmalloc() is a system API that allocates physical memory 
    //and returns appropriate starting virtual/logical address of 
    //physical memory - there are several rules - for the time being,
    //let us use the default calling conventions !!!
    //you need not create objects and maintain a list in assignment 2
    //you will be using system objects and system data structures
    //in assignment 2
   for (i=0;i<100;i++) {
       mydrv_new = kmalloc(sizeof(struct _mydrv_struct), GFP_KERNEL);
    //check errors
    sprintf(mydrv_new->info, "Node No: %d\n", i);
    list_add_tail(&mydrv_new->list, &mydrv_list);
  }

  printk("we are in init function of the module\n");  //2
  return 0;
}

static void mydrv_exit(void)
{
  //incomplete
  struct _mydrv_struct *p,*n;
  list_for_each_entry_safe(p,n, &mydrv_list, list) 
      kfree(p);

   remove_proc_entry("readme", NULL);
   printk("mydrv_exit just executed\n");    //3

}

module_init(mydrv_init);
module_exit(mydrv_exit);


//add other macros as needed 


