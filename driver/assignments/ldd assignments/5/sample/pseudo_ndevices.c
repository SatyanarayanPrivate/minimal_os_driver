#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/vmalloc.h>
#include<linux/slab.h>
#include<linux/fs.h>
#include<linux/list.h>
#include<linux/cdev.h>
#include<linux/major.h>
#include<linux/kfifo.h>
#include<linux/wait.h>
#include<linux/errno.h>
#include<linux/device.h>
#include<linux/moduleparam.h>
#include<asm/uaccess.h>
#include<linux/fcntl.h>
#include<linux/interrupt.h>
#include<linux/sched.h>
#include <linux/ioctl.h>
#define PLP_KMEM_BUFSIZE 1024 /* 1MB internal buffer */
#define MYIOC_TYPE 'k'
/* global variables */
int ndevices = 1;
module_param(ndevices,int,S_IRUGO);

static dev_t plp_kmem_dev;	

static int __init plp_kmem_init(void);
static void __exit plp_kmem_exit(void);

static int plp_kmem_open(struct inode *inode, struct file *file);
static int plp_kmem_release(struct inode *inode, struct file *file);
static ssize_t plp_kmem_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos);
static ssize_t plp_kmem_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos);
static long mycdrv_unlocked_ioctl (struct file *, unsigned int, unsigned long );

typedef struct priv_obj1{
	unsigned char *buff;
	struct kfifo *kfifo;
	spinlock_t my_lock;
	wait_queue_head_t rd_queue;
	wait_queue_head_t wr_queue;
	struct list_head list;
	struct cdev cdev;
}P_OBJ1;

LIST_HEAD(dev_list);

static struct file_operations plp_kmem_fops = {
	.read		= plp_kmem_read,
	.write		= plp_kmem_write,
	.open		= plp_kmem_open,
	.unlocked_ioctl = mycdrv_unlocked_ioctl,
	.release	= plp_kmem_release,
	.owner		= THIS_MODULE,
};


static int plp_kmem_open(struct inode *inode, struct file *file)
{
  	P_OBJ1 *obj;
  	obj = container_of(inode->i_cdev,P_OBJ1, cdev);
  	file->private_data = obj;
	return 0;
}

static int plp_kmem_release(struct inode *inode, struct file *file)
{

#ifdef PLP_DEBUG
	printk(KERN_DEBUG "plp_kmem: device closed.\n");
#endif

	return 0;
}
static ssize_t plp_kmem_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
    P_OBJ1 *dev= file->private_data;
	int ret=0, ret1, bytes=0;
 	bytes = kfifo_len(dev->kfifo);
	ret = kfifo_size(dev->kfifo) - bytes;
	//printk("count = %d, *ppos = %d, ppos = %x, buf = %x \n", count, fpos,ppos, buf);
	if(bytes==kfifo_size(dev->kfifo))
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
        wait_event_interruptible(dev->wr_queue,(kfifo_size(dev->kfifo)-kfifo_len(dev->kfifo)) > 0);
		}
	}	
	//ret1 = kfifo_in((dev->kfifo), (char __user *)buf, count);
	ret1 = kfifo_in_locked(dev->kfifo,(char __user *)buf,count,dev->my_lock); 
	if(ret1<0) return -EFAULT;	
	wake_up_interruptible_all(&(dev->rd_queue));

	printk("no of bytes placed in fifo %d \n", ret1);
	printk("data sent in fifo %s \n", buf);	
	
	return ret1;

}

static ssize_t plp_kmem_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)  //*ppos-offset  ; count = 4096 buffer
{  	
	P_OBJ1 *dev= file->private_data;
	int bytes;
	dev = file->private_data;
	printk("inside read call...\n");

	bytes = kfifo_len(dev->kfifo);  //no of used bytes

	if(bytes==0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
        wait_event_interruptible(dev->rd_queue,kfifo_len(dev->kfifo) != 0);
		
		}
	}
	if(access_ok(VERIFY_WRITE,(void __user*)buf,(unsigned long)count))  //copy to user
	{
//		bytes = kfifo_out(dev->kfifo,buf,count);
		bytes = kfifo_out_locked(dev->kfifo, buf, count, dev->my_lock);
		if(bytes<0) return -EFAULT;
		wake_up_interruptible_all(&(dev->wr_queue));
		return bytes;
	}
	else
		return -EFAULT;
}

static long
mycdrv_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	P_OBJ1 *dev= file->private_data;
    int size=0, size_ff, rc, direction;
    void __user *ioargp = (void __user *)arg;
	
    if (_IOC_TYPE (cmd) != MYIOC_TYPE) {
        printk (KERN_INFO " got invalid case, CMD=%d\n", cmd);
        return -EINVAL;
    }
	size_ff = kfifo_size(dev->kfifo) - kfifo_len(dev->kfifo) ;
    direction = _IOC_DIR (cmd);
    size = _IOC_SIZE(cmd);

    switch (direction) { 

    case _IOC_READ:	
	rc = copy_to_user (ioargp, &size_ff, size);
	
	printk("fifo size : %d \n", size_ff);
        return rc;
        break;
   

    default:
        printk (KERN_INFO " got invalid case, CMD=%d\n", cmd);
        return -EINVAL;
    }
}



P_OBJ1 *my_dev;

static int __init plp_kmem_init(void)
{
	int ret;
	int i,j;
	
   
	if (alloc_chrdev_region(&plp_kmem_dev, 0, ndevices, "pseudo_driver"))
		goto error;
	for(i=0;i<ndevices;i++)	
	{
		printk("MAJOR and MINOR no. are %d and %d \n",MAJOR(plp_kmem_dev+i), MINOR(plp_kmem_dev+i));
	my_dev = kmalloc(sizeof(P_OBJ1),GFP_KERNEL);
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
				unregister_chrdev_region(plp_kmem_dev,ndevices); 
				return -ENOMEM;
			}
		}	

       	my_dev->kfifo = kmalloc(PLP_KMEM_BUFSIZE,GFP_KERNEL);  //allocating mem to kfifo
		if(my_dev->kfifo==NULL)
		{
			printk("Error \n");
			if(i >= 1)				//error checking for second device onwards
			{	kfree(my_dev); 
				//err |= -ENOMEM;
				goto error;
			}
			else
			{	kfree(my_dev); 
				unregister_chrdev_region(plp_kmem_dev,ndevices); 
				return -ENOMEM;
			}
		}

	
		my_dev->buff = kmalloc(PLP_KMEM_BUFSIZE, GFP_KERNEL);  //allocating mem to buf

		if(my_dev->buff == NULL)
		{
			printk("Error in allocating memory for device buffer....\n");
			if(i >= 1)
			{

				kfree(my_dev); 
				kfree(my_dev->kfifo);
				goto error;
			}
			else{
             
			    kfree(my_dev);
				kfree(my_dev->kfifo);
				unregister_chrdev_region(plp_kmem_dev,ndevices); 
				return -ENOMEM;
			}	
		}

		list_add_tail(&my_dev->list,&dev_list);				//add to list queue of device
		printk(" %d: list add tail\n", i);

		spin_lock_init(&(my_dev->my_lock));
	
		kfifo_init(my_dev->kfifo, (void*)my_dev->buff, PLP_KMEM_BUFSIZE);
		
		init_waitqueue_head(&my_dev->wr_queue);
		init_waitqueue_head(&my_dev->rd_queue);

       	cdev_init(&my_dev->cdev,&plp_kmem_fops); 
		my_dev->cdev.owner = THIS_MODULE;      
    	
		my_dev->cdev.ops = &plp_kmem_fops; /* file up fops */
	
        ret = cdev_add(&my_dev->cdev, plp_kmem_dev+i, 1);
		if(ret<0)
		{
			printk("Error in cdev adding....\n");
			
			if(i >= 1)
			{
				kfree(my_dev->kfifo);
			   	kfree(my_dev->buff);
				kfree(my_dev);
				unregister_chrdev_region(plp_kmem_dev,ndevices);
				//err |= -EBUSY;
				goto error; 
			}
			else
			{
				kfree(my_dev->kfifo);
			    kfree(my_dev->buff);
				kfree(my_dev);
				unregister_chrdev_region(plp_kmem_dev,ndevices);
				return -EBUSY;
			}
		}

	}

	printk(KERN_INFO "plp_kmem: loaded.\n");

	return 0;

error:
	printk(KERN_ERR "plp_kmem: cannot register device.\n");

	return 1;
}

static void __exit plp_kmem_exit(void)
{	
	int i;
	P_OBJ1 *p,*n1;
	list_for_each_entry_safe(p,n1,&dev_list, list) 
      	{ cdev_del(&(p->cdev)); kfree(p->buff);kfree(p->kfifo);kfree(p);  }
	
	unregister_chrdev_region(plp_kmem_dev, ndevices);
	 
	printk(KERN_INFO "plp_kmem: unloading.\n");
}


module_init(plp_kmem_init);
module_exit(plp_kmem_exit);


MODULE_DESCRIPTION("Demonstrate kernel memory allocation");

MODULE_ALIAS("memory_allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");
