#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/version.h>
#include<linux/init.h>
#include<linux/device.h>
#include<linux/pci.h>
#include<linux/ioport.h>
#include<asm/unistd.h>
#include<linux/slab.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<asm/uaccess.h>
#include<asm/io.h>
#include<linux/kdev_t.h>
#include<asm/fcntl.h>
#include<linux/sched.h>
#include<linux/wait.h>
#include<linux/errno.h>
#include<linux/kfifo.h>
#include<asm/irq.h>
#include<asm/errno.h>
#include<asm/ioctl.h>
#include<linux/string.h>
#include<linux/interrupt.h>
#include<linux/cdev.h>

#define MAX_BUFFSIZE (5*1024)
//#define err 0

static dev_t pcdd_dev;
//static struct cdev *pcdd_cdev;

static int __init pcdd_init(void);
static void __exit pcdd_exit(void);

static int pcdd_open(struct inode *inode,struct file *file);
static int pcdd_release(struct inode *inode,struct file *file);
static ssize_t pcdd_read(struct file *file,const char __user *buff,ssize_t count,loff_t *pos);
static ssize_t pcdd_write(struct file *file,const char __user *buff,ssize_t count,loff_t *pos);


typedef struct priv_obj1
{
	struct list_head list;
	dev_t dev;
	struct cdev cdev;
	unsigned char *buff;  //kernel buffer used with kfifo object
	struct kfifo *kfifo;  //kfifo object ptr 
	spinlock_t my_lock;
	wait_queue_head_t queue; //you may need more than one wq 
}C_DEV;

static struct file_operations pcdd_fops = {
	.open    = pcdd_open,
	.read    = pcdd_read,
	.write   = pcdd_write,
	.release = pcdd_release,
	.owner   = THIS_MODULE,
};

LIST_HEAD(dev_list);
int ndevices=1;

static int pcdd_open(struct inode *inode,struct file *file)
{
        //must complete open method - ???

	printk("Driver : opend device\n");
	return 0;
}

static int pcdd_release(struct inode *inode,struct file *file)
{
	printk("Driver : closed device\n");
	return 0;
}

static ssize_t pcdd_read(struct file *file,const char __user *buff,ssize_t count,loff_t *pos)
{
	C_DEV *dev;
	int bytes;

        //by using private_data and private object combination,
        //we can pass our device instance's object to our driver's
        //methods - this ensures that our driver methods are 
        //reentrant 
        //
        //meaning, our device instances can be concurrently accessed 
        //by different processes/threads of the system and this
        //can be done with true concurrency as our driver methods
        //are reentrant !!!
         
	dev = file->private_data;
	printk("inside read call...\n");

        //dump_stack(); //verify that the flow of execution 
                        //is as expected and parameters are as expected !!!

        //refer to LKD/3 for kfifos (known as queues, in chapter 6 !!!
        //also refer to <KSRC>/include/linux/kfifo.h for more details !!!
        //also refer to <KSRC>/kernel/kfifo.c for more details !!!

	bytes = __kfifo_len(dev->kfifo);

	if(bytes==0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{

       //whenever wait_event_interruptible() is invoked, it does the
       //following:
       //-checks the condition(a C expression) provided - if it 
       // is true, do not block and just return - if the C expression,
       // is false, block the current process in the wq provided as
       // first parameter of wait_event_interruptible() !!!
       //
       // after the current process is blocked in the wq, scheduler
       // is invoked and another prcocess is scheduled !!!
       //
       //some time in the future, this process will be woken up 
       //by another method of the driver, when data is available -
       //typically, such a method may be an interrupt handler, 
       //bottom half handler(softirq / tasklet) or it may be 
       //another operation of file_operations{} table of this
       //driver !!!

       //after wake-up, our process will be rescheduled and it 
       //will resume from the wait_event_interruptible() - as per 
       //rules of this system API, it will once again verify 
       //the condition - C expression - if the condition is false, 
       //process will once again be blocked - if the condition 
       //is true, it will allow the process to proceed by 
       //returning !!!!

        wait_event_interruptible(dev->queue,(__kfifo_len(dev->kfifo) != 0));
		}
	}
        //access_ok() validates user space buffer ptr and its locations
        //if it is a valid user-space buffer, return 1 - otherwise, 0 

	if(access_ok(VERIFY_WRITE,(void __user*)buff,(unsigned long)count))
	{
		bytes = __kfifo_get(dev->kfifo,(unsigned char*)buff,count);
		return bytes;
	}
	else
		return -EFAULT;

}
//in the first driver assignment, wake up responsibilty will be 
//that of write() method for processes blocked in read() method
//and vice-versa - however, this will not be true in further drivers,
//where other mechanisms are used for wake-up of blocked processes !!!!
static ssize_t pcdd_write(struct file *file,const char __user *buff,ssize_t count,loff_t *pos)
{
	P_OBJ1 *dev;
	int val;
	dev = file->private_data;
	printk("inside write call...\n");

        //must complete write method - ??
         

}

module_param(ndevices,int,S_IRUGO);

C_DEV *my_dev;
static int __init pcdd_init(void)
{	
	int i,j;
	if(alloc_chrdev_region(&pcdd_dev,0,ndevices,"pseudo_driver"))	//creating device ids for each device & storing it into corresponding device pcdd_dev
	{
		printk("Error in device creating.....\n");
		//err |= EBUSY;
		return -EBUSY;
	}
	printk("1: alloc_chrdrv_region end\n");
	for(i=0;i<ndevices;i++)
	{
		my_dev = kmalloc(sizeof(C_DEV),GFP_KERNEL);			//create memory for pseduo devices
		if(my_dev==NULL)
		{
			printk("Error in creating devices....\n");
			if(i >= 1)				//error checking for second device onwards
			{
				//err |= -ENOMEM;
				goto error;
			}
			else
			{
				unregister_chrdev_region(pcdd_dev,ndevices); 
				return -ENOMEM;
			}
		}
		printk("2 %d: kmallic for my_dev\n", i);
		list_add_tail(&my_dev->list,&dev_list);				//add to list queue of device
		printk("3 %d: list add tail\n", i);
		
		my_dev->buff = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);
		if(my_dev == NULL)
		{
			printk("Error in allocating memory for device buffer....\n");
			if(i >= 1)
			{

				kfree(my_dev); 
				//err |= -ENOMEM;
				goto error;
			}
			else{
             
			    kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices); 
				return -ENOMEM;
			}	
		}
		printk("4 %d: kmalloc buffer\n", i);
		spin_lock_init(&(my_dev->my_lock));
		printk("5: %d spin_lock init\n", i);
		
		my_dev->kfifo = kfifo_init(my_dev->buff,MAX_BUFFSIZE,GFP_KERNEL,&(my_dev->my_lock));

		if(my_dev->kfifo == NULL)
		{
			printk("Error in initializing kfifo.....\n");
			if(i >= 1)
			{
			    kfree(my_dev->buff);
			    kfree(my_dev);
				//err |= -ENOMEM;
				goto error;
			}
			else
			{
			    kfree(my_dev->buff);
			    kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices); 
				return -ENOMEM;
			}
		}
		printk("6: %d kfifo init\n", i);
			
		cdev_init(&my_dev->cdev,&pcdd_fops);			//initialse current device's operation with our written file operations
		printk("7: %d cdev init\n", i);
		kobject_set_name(&(my_dev->cdev.kobj),"device%d",i);	//give name to current device
		printk("8: %d kobj_set_name\n", i);
		
		my_dev->cdev.ops = &pcdd_fops;
		printk("9: %d my_dev->cdev.ops\n", i);
		
		if(cdev_add(&my_dev->cdev,pcdd_dev+i,1)<0)
		{
			printk("Error in cdev adding....\n");
			kobject_put(&(my_dev->cdev.kobj));
			if(i >= 1)
			{
				kfifo_free(my_dev->kfifo);
			    kfree(my_dev->buff);
				kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices);
				//err |= -EBUSY;
				goto error; 
			}
			else
			{
				kfifo_free(my_dev->kfifo);
			    kfree(my_dev->buff);
				kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices);
				return -EBUSY;
			}
		}
		printk("10: %d cdev_add\n", i);

	}
	printk(KERN_INFO "pcdd : loaded\n");
	return 0;

	error:
	{
                //must complete error handling 

		return -ENOMEM; //use a variable to set the error code
						//return the same variable from each point of error 
	}
}

static void __exit pcdd_exit(void)
{
	int i;


        //must complete this method ??
	
	printk("pcdd : unloading\n");
}

module_init(pcdd_init);
module_exit(pcdd_exit);

MODULE_DESCRIPTION("Pseudo Device Driver");
MODULE_ALIAS("memory allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");



