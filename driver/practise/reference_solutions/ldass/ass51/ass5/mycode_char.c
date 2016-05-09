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
#include<linux/spinlock.h>

#define MAX_BUFFSIZE (2*1024)

static dev_t pcdd_dev;
//static struct cdev *pcdd_cdev;

static int __init pcdd_init(void); //prototypes
static void __exit pcdd_exit(void);

static int pcdd_open(struct inode *inode,struct file *file);
static int pcdd_release(struct inode *inode,struct file *file);
static ssize_t pcdd_read(struct file *file,char __user *buff,size_t count,loff_t *pos);
static ssize_t pcdd_write(struct file *file,const char __user *buff,size_t count,loff_t *pos);


typedef struct priv_obj1
{
	struct list_head list;
	dev_t dev;
	struct cdev cdev;
	unsigned char *buff;  //kernel buffer used with kfifo object
	struct kfifo kfifo;  //kfifo object ptr 
	spinlock_t my_lock;
	wait_queue_head_t queue; 
}C_DEV;

static struct file_operations pcdd_fops = {
	.open    = pcdd_open,
	.read    = pcdd_read,
	.write   = pcdd_write,
	.release = pcdd_release,
	.owner   = THIS_MODULE
};

LIST_HEAD(dev_list);
int ndevices=1;

C_DEV *my_dev;
static int pcdd_open(struct inode *inode,struct file *file)
{
    C_DEV *dev; 
    dev = container_of(inode->i_cdev,C_DEV,cdev);
    file->private_data = dev; /* for other methods */
    /* now trim to 0 the length of the device if open was write-only */
       
    printk("Driver : opened device\n");
    return 0;
}

static int pcdd_release(struct inode *inode,struct file *file)
{
	printk("Driver : closed device\n");
	return 0;
}

static ssize_t pcdd_read(struct file *file,char __user *buff,size_t count,loff_t *pos)
{
	C_DEV *dev;
	unsigned int bytes;

                
	dev = file->private_data;
	printk("inside read call...\n");

        dump_stack(); //verify that the flow of execution 
                        //is as expected and parameters are as expected !!!

        
	//bytes = kfifo_len(&(dev->kfifo));//returns no. of used bytes(edited)

	if(kfifo_is_empty(&dev->kfifo))
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
			wait_event_interruptible(dev->queue,(kfifo_len(&(dev->kfifo)) != 0)); //blocking if nothing to read
		}
	}
        

	if(access_ok(VERIFY_WRITE,(void __user*)buff,(unsigned long)count))
	{
		bytes= kfifo_out_locked(&(dev->kfifo),(unsigned char*)buff,(unsigned int)count,&(dev->my_lock));
		if(bytes>0)//read
		{
			printk("Written %d bytes\n",bytes);
			return bytes;
		}
		else
		return -EFAULT;
	}
	else
		return -EFAULT;

}
static ssize_t pcdd_write(struct file *file,const char __user *buff,size_t count,loff_t *pos)
{
	C_DEV *dev;
	unsigned int val;
	dev = file->private_data;
	printk("inside write call...\n");
	
	val=kfifo_in_locked(&(dev->kfifo),(unsigned char*)buff,(unsigned int)count, &(dev->my_lock));
	if (val==0)
	{
		return -EFAULT;
	}
	printk("val=%d",val);
	return 0; //success
}

module_param(ndevices,int,S_IRUGO);

static int __init pcdd_init(void)
{	
	int i;
	if(alloc_chrdev_region(&pcdd_dev,0,ndevices,"pseudo_driver"))
	{
		printk("Error in device creating.....\n");
		return -EBUSY;
	}
	
	printk("1: alloc_chrdrv_region end\n");
	for(i=0;i<ndevices;i++)
	{
		my_dev = kmalloc(sizeof(C_DEV),GFP_KERNEL);//create memory for pseduo devices
		if(my_dev==NULL)
		{
			printk("Error in creating devices....\n");
			if(i >= 1)
			{
				goto error;
			}
			else
			{
				unregister_chrdev_region(pcdd_dev,ndevices); 
				return -ENOMEM;
			}
		}
		printk("2 %d: kmalloc for my_dev\n", i);
		list_add_tail(&(my_dev->list),&dev_list);	//add to list queue of device
		printk("3 %d: list add tail\n",i);
		
		my_dev->buff = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);
		if(my_dev == NULL)
		{
			printk("Error in allocating memory for device buffer....\n");
			if(i >= 1)
			{

				kfree(my_dev); 
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
		
		kfifo_init(&(my_dev->kfifo),my_dev->buff,MAX_BUFFSIZE);

	/*	if(&(&(my_dev->kfifo)) == NULL)
		{
			printk("Error in initializing kfifo.....\n");
			if(i >= 1)
			{
			    kfree(my_dev->buff);
			    kfree(my_dev);
			
				goto error;
			}
			else
			{
			    kfree(my_dev->buff);
			    kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices); 
				return -ENOMEM;
			}
		}*/
		printk("6: %d kfifo init\n", i);
			
		cdev_init(&(my_dev->cdev),&pcdd_fops); 			
		printk("7: %d cdev init\n", i);
		kobject_set_name(&(my_dev->cdev.kobj),"device%d",i);
		printk("8: %d kobj_set_name\n", i);
		my_dev->cdev.ops = &pcdd_fops;
		printk("9: %d my_dev->cdev.ops\n", i);
		
		if(cdev_add(&(my_dev->cdev),pcdd_dev+i,1)<0)
		{
			printk("Error in cdev adding....\n");
			kobject_put(&(my_dev->cdev.kobj));
			if(i >= 1)
			{
				kfifo_free(&(my_dev->kfifo));
			    kfree(my_dev->buff);
				kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices);
				goto error; 
			}
				
			else
			{
				kfifo_free(&(my_dev->kfifo));
			    kfree(my_dev->buff);
				kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices);
				return -EBUSY;
			}
			error:
			{
				return -ENOMEM;
			}
		}
		printk("10: %d cdev_add\n", i);

	}
	printk(KERN_INFO "pcdd : loaded\n");
	return 0;

	
}

static void __exit pcdd_exit(void)
{
int i;
	for(i=0;i<ndevices;i++)
	{
		cdev_del(&(my_dev->cdev));
		kfifo_free(&(my_dev->kfifo));
		kfree(my_dev);
	}
	unregister_chrdev_region(pcdd_dev,ndevices);

  	printk("pcdd : unloading\n");
}

module_init(pcdd_init);
module_exit(pcdd_exit);

MODULE_DESCRIPTION("Pseudo Device Driver");
MODULE_ALIAS("memory allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");



