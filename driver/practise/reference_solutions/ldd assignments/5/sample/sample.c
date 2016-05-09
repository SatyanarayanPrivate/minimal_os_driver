#include <linux/vmalloc.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/version.h>
#include<linux/init.h>
#include<linux/device.h>
#include<linux/pci.h>
#include<linux/ioport.h>
#include<asm/unistd.h>
#include<linux/slab.h>
#include<linux/fs.h>   //
#include<linux/types.h>
#include<asm/uaccess.h>
#include<asm/io.h>
#include<linux/kdev_t.h>
#include<asm/fcntl.h>
#include<linux/sched.h>
#include<linux/wait.h>  //
#include<linux/kfifo.h>
#include<linux/list.h>
#include<asm/errno.h>
#include<asm/ioctl.h>
#include<linux/string.h>
#include<linux/interrupt.h>
#include<linux/cdev.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/log2.h>


#define MAX_BUFFSIZE (2 * 1024)
static dev_t mydev;
int ndevice;

//static int __init char_dev_init(void);
//static void __exit char_dev_exit(void);

static int pcdd_open(struct inode *inode,struct file *file);
static int pcdd_release(struct inode *inode,struct file *file);
static ssize_t pcdd_read(struct file *file,const char __user *buff,ssize_t count,loff_t *pos);
static ssize_t pcdd_write(struct file *file,const char __user *buff,ssize_t count,loff_t *pos);


typedef struct priv_obj{
		struct list_head list;	
		dev_t dev;
		struct cdev cdev;
		void *buff;
		struct kfifo kfifo;
		spinlock_t my_lock;
		wait_queue_head_t queue1,queue2;  //use 2 queues - one for reading and one for writing 
}c_dev;


static struct file_operations pcdd_fops = {
		.open    = pcdd_open,
		.read    = pcdd_read,
		.write   = pcdd_write,
		.release = pcdd_release,
		.owner   = THIS_MODULE,
};


LIST_HEAD(dev_list);
c_dev *pri_dev;

//-------------------------------------------------------------------------------------------

static int pcdd_open(struct inode *inode,struct file *file)
{
	c_dev *obj;					 //private object type
	obj = container_of(inode->i_cdev, c_dev, cdev);  //container_of(ptr, type, member)
	file->private_data = obj;        		 //attach private data of open file to our pri obj,
	//here the open is dummy - not always
	#ifdef PLP_DEBUG
		printk(KERN_DEBUG "pcdd: opened device.\n");
	#endif
	
	printk("Driver : opend device\n");
	return 0;
}

//-------------------------------------------------------------------------------------------

static int pcdd_release(struct inode *inode,struct file *file)
{
	#ifdef PLP_DEBUG
	printk(KERN_DEBUG "pcdd: device closed.\n");
	#endif	
	
	printk("Driver : closed device\n");
	return 0;
}
//-------------------------------------------------------------------------------------------
static ssize_t pcdd_read(struct file *file,const char __user *buff,ssize_t count,loff_t *pos)
{
	c_dev *dev;
	int bytes, ret;
	char buffer[MAX_BUFFSIZE];
	dev = file->private_data;
	printk("inside read call...\n");

	bytes = kfifo_len(&(dev->kfifo));
	printk("kfifo_len in read = %d \n", kfifo_len(&(dev->kfifo)));
	//buffer = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);
	if(bytes==0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		
		else
		{
        		wait_event_interruptible(dev->queue1,kfifo_len(&(dev->kfifo)) != 0);
		}
	}
	if(access_ok(VERIFY_WRITE,(void __user*)buff,(unsigned long)count))
	{
		bytes = kfifo_out(&(dev->kfifo),(unsigned char *)buff,bytes);
		printk("kfifo_avail in read = %d \n", kfifo_avail(&(dev->kfifo)));		
		printk("msg in read = %s \n", buffer);		
		//ret = copy_to_user((unsigned char *)buff,buffer,count);		
		
		//*pos=*pos+bytes;
		 //kfree(buffer);		
		
	}
	else
		return -EFAULT;
	
	return bytes;
	
}
//-------------------------------------------------------------------------------------------
static ssize_t pcdd_write(struct file *file, const char __user *buff, ssize_t count, loff_t *pos)
{
	c_dev *dev;
	int bytes, ret;
	char buffer[MAX_BUFFSIZE];
	dev = file->private_data;
	printk("inside write call --\n");
	bytes = kfifo_avail(&(dev->kfifo));                     //must use kfifo_avail():
	//bytes = count;
	printk("kfifo_len in write = %d \n", kfifo_len(&(dev->kfifo)));
	printk("kfifo_avail in write = %d \n", kfifo_avail(&(dev->kfifo)));
	//buffer = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);
	if(bytes == 0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
			
		else
		{
			wait_event_interruptible(dev->queue2,(kfifo_avail(&(dev->kfifo)) != 0));
		}
	}
	if(access_ok(VERIFY_READ,(void __user*)buff,(unsigned long)count))
	{
                //if you have used access_ok(), there is no need for copy_*_user APIs 

		//ret = copy_from_user(buffer, (unsigned char *)buff, bytes);
			
		printk (KERN_ALERT "After copy_from_user\n");		
		printk("kfifo_avail in write = %d \n", kfifo_avail(&(dev->kfifo)));
		printk("msg in write = %s \n", buffer);		
		bytes = kfifo_in(&(dev->kfifo),(unsigned char *)buff, bytes);	//copy from user to kfifo
		printk (KERN_ALERT "After kfifo_in\n");		
		
		printk("kfifo_len in write = %d \n", kfifo_len(&(dev->kfifo)));
		
	}
	else
		
		return -EFAULT;
	printk (KERN_ALERT "Exited write method. No of bytes written %d\n", bytes);	
	return bytes;
}
	
//-------------------------------------------------------------------------------------------


module_param(ndevice,int,S_IRUGO);


static int __init char_dev_init(void)
{
	int i, j;
	j=alloc_chrdev_region(&mydev,0,ndevice,"pseudo_driver");//with the help of this method we 									//create a device id with the help of 									//which we can extract the major & 									//minor nos
	printk("device id = %d\n",mydev);
	if(j==0)
	{
		printk("device get its major no.\n");
	}
	for(i=0;i<ndevice;i++)
	{	
		printk("major = %d\n",MAJOR(mydev + i));
		printk("minor = %d\n",MINOR(mydev + i));
	}
	for(i=0;i<ndevice;i++)
	{		
		pri_dev=kmalloc(sizeof(c_dev),GFP_KERNEL);//allocating memory to the private object
		if(pri_dev==NULL)
		{
			printk("error in memory allocation\n");
			if(i >= 1)				//error checking for second device onwards
			{
				goto error;
			}
			else
			{
				unregister_chrdev_region(mydev,ndevice); 
				return -ENOMEM;
			}
		}
	

		printk("2 : kmalloc for my_dev\n");
		list_add_tail(&pri_dev->list,&dev_list);	//add to list queue of device
		printk("3 : list add tail\n");

		pri_dev->buff = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);	
		if(pri_dev == NULL)
		{
			printk("Error in allocating memory for device buffer....\n");
			if(i >= 1)
			{

				kfree(pri_dev); 
				goto error;
			}
			else
			{
				kfree(pri_dev);
				unregister_chrdev_region(mydev,ndevice); 
				return -ENOMEM;
			}	
		}
		printk("4 : kmalloc buffer\n");
		
		spin_lock_init(&(pri_dev->my_lock));	
		printk("5:  spin_lock in  it\n");
		
		kfifo_init(&(pri_dev->kfifo),pri_dev->buff,MAX_BUFFSIZE );
		printk("6:  kfifo init\n");
		
		init_waitqueue_head(&(pri_dev->queue1));	//initilization of queue for read
		init_waitqueue_head(&(pri_dev->queue2));	//initilization of queue for write
		printk("7: queue init\n");
	
		cdev_init(&pri_dev->cdev,&pcdd_fops);     
		printk("8: cdev init\n");
		kobject_set_name(&(pri_dev->cdev.kobj),"device%d",i);	//give name to current device
		//printk("%s\n",&(pri_dev->cdev.kobj));		
		printk("9:  kobj_set_name\n");
		
		pri_dev->cdev.ops = &pcdd_fops;
		printk("10:  pri_dev->cdev.ops\n");	
		
		if(cdev_add(&pri_dev->cdev,mydev+i,1)<0)
		{
			printk("Error in cdev adding....\n");
			kobject_put(&(pri_dev->cdev.kobj));
			if(i >= 1)
			{
				kfifo_free(&(pri_dev->kfifo));
			    	//kfree(pri_dev->buff);
				kfree(pri_dev);
				unregister_chrdev_region(mydev,ndevice);
				goto error; 
			}
			else
			{
				kfifo_free(&(pri_dev->kfifo));
			    	//kfree(pri_dev->buff);
				kfree(pri_dev);
				unregister_chrdev_region(mydev,ndevice);
				return -EBUSY;
			}
		}
		printk("11: cdev_add\n");
		printk("i am in init\n");
		
		return 0;
		error:   //error checking is incomplete 
		{
			struct list_head *cur, *prev;
			list_for_each_safe(cur,prev,&(dev_list))
			{
				pri_dev = container_of(cur, c_dev,list);
				kfree(&(pri_dev->kfifo));		
				kfree(pri_dev);
				printk("major = %d\n",MAJOR(mydev));
				printk("minor = %d\n",MINOR(mydev));
			}
			unregister_chrdev_region(mydev,ndevice);
		}
	}
}

static void char_dev_exit(void)
{	
	//error checking is incomplete 
	struct list_head *cur, *prev;
	list_for_each_safe(cur,prev,&(dev_list))
	{
		pri_dev = container_of(cur, c_dev,list);
		kfree(&(pri_dev->kfifo));		
		kfree(pri_dev);
		printk("major = %d\n",MAJOR(mydev));
		printk("minor = %d\n",MINOR(mydev));
			
		printk("i am exit from the pc\n");
	}
	unregister_chrdev_region(mydev,ndevice);
}

module_init(char_dev_init);
module_exit(char_dev_exit);

MODULE_DESCRIPTION("pseudo device driver");
MODULE_ALIAS("memory allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");

