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
#include<linux/kobject.h>
#define MAX_BUFFSIZE (2*1024)

 int ndevices;

static dev_t my_dev;



typedef struct P_OBJ
{
	struct list_head list;
	dev_t dev;
	struct cdev cdev;
	unsigned char *buff;
	struct kobject *kobj;
	struct kfifo kfifo;
	spinlock_t my_lock;
	wait_queue_head_t queue1,queue2;		//use 2 queue  1 for write and 1 for read
}C_DEV;

static int my_open(struct inode *inode,struct file *file)
{
       
	C_DEV *obj;
	printk("in open method\n");
 	obj = container_of(inode->i_cdev,C_DEV, cdev);

  	file->private_data = obj;
	dump_stack();

	printk("Driver : opend device\n");
	return 0;
}

static ssize_t my_read(struct file *file,char __user *buff,size_t count,loff_t *pos)
{
	C_DEV *dev;
	int bytes;
	dev = file->private_data;
	printk("inside read call...\n");

	bytes = kfifo_len(&(dev->kfifo));

	if(bytes==0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
       			 wait_event_interruptible(dev->queue1,(kfifo_len(&(dev->kfifo)) != 0));
		}
	}
	if(access_ok(VERIFY_WRITE,(void __user*)buff,(unsigned long)count))
	{
		bytes =kfifo_out(&(dev->kfifo),(unsigned char*)buff,count);
	//	return bytes;
	}
	else
		return -EFAULT;
		
	return bytes;

}

static ssize_t my_write(struct file *file,const char __user *buff,size_t count,loff_t *pos)
{
	C_DEV *dev;
	int val;
	dev = file->private_data;
	printk("inside write call...\n");
	
	val = kfifo_avail(&(dev->kfifo));

	if(val==0)
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
		val =kfifo_in(&(dev->kfifo),(unsigned char*)buff,count);
//		return val;
	}
	else
		return -EFAULT;

	return val;
}


static int my_release(struct inode *inode,struct file *file)
{
	printk("Driver : closed device\n");
	return 0;
}


//static int my ;
//struct kobj_attribute *attr;

static ssize_t my_show(struct kobject kobj,struct kobj_attribute *attr, char *buf)
{
	int show ;
	show = MAJOR(my_dev)
	return sprintf(buf,"%d\n",show);
}

static ssize_t my_store(struct kobject kobj,struct kobj_attribute *attr, const char *buf, size_t count)
{	
	
	sscanf(buf,"%d",&my);
	return count;
}

 struct kobj_attribute my_attribute = __ATTR(my,0666,my_show,my_store);

static struct attribute *attrs[] = {&my_attribute.attr, NULL};

static struct attribute_group attr_group ={ .attrs = attrs, NULL };


module_param(ndevices,int,S_IRUGO);

LIST_HEAD(dev_list);


C_DEV *dev;

static struct file_operations my_fops = {
	.open    = my_open,
	.read    = my_read,
	.write   = my_write,
	.release = my_release,
	.owner   = THIS_MODULE,
};


//static struct kobject *my_kobject;


static int __init my_init(void)
{	
	
	int i,retval = 0;
	if(alloc_chrdev_region(&my_dev,0,ndevices,"pseudo_driver"))
	{
		printk("Error in device creating.....\n");
		return -EBUSY;
	}

	printk("1: alloc_chrdrv_region end\n");
	printk("major no = %d\n",MAJOR(my_dev));
	for(i=0;i<ndevices;i++)
	{
		dev = kmalloc(sizeof(C_DEV),GFP_KERNEL);	
		if(dev==NULL)
		{
			printk("Error in creating devices....\n");
			if(i >= 1)				
			{
				goto error;
			}
			else
			{
				unregister_chrdev_region(my_dev,ndevices); 
				return -ENOMEM;
			}
		}

		printk("2 %d: kmalloc for my_dev\n", i);
		
		list_add_tail(&dev->list,&dev_list);

		printk("3 %d: list add tail\n", i);
		
		dev->buff = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);
		
		if(dev == NULL)
		{
			printk("Error in allocating memory for device buffer....\n");
			if(i >= 1)
			{

				kfree(dev); 
				goto error;
			}

			else{
             
			    kfree(dev);
				unregister_chrdev_region(my_dev,ndevices); 
				return -ENOMEM;
			}	
		}
		
		printk("4 %d: kmalloc buffer\n", i);

		spin_lock_init(&(dev->my_lock));

		printk("5: %d spin_lock init\n", i);

                 kfifo_init(&(dev->kfifo),dev->buff,MAX_BUFFSIZE);
		 
		(dev->kobj) = kobject_create_and_add("kobject_my",kernel_kobj);


		retval = sysfs_create_group(dev->kobj,&attr_group);
		if(retval)
			kobject_put(dev->kobj);

                printk("6: %d kfifo init\n", i);
			
		cdev_init(&dev->cdev,&my_fops);			
		printk("7: %d cdev init\n", i);
		init_waitqueue_head(&(dev->queue1));
		init_waitqueue_head(&(dev->queue2));
	
		kobject_set_name(&(dev->cdev.kobj),"device%d",i);	
									                                              	
		printk("8: %d kobj_set_name\n", i);
		
		dev->cdev.ops = &my_fops;
		printk("9: %d dev->cdev.ops\n", i);
		
		if(cdev_add(&dev->cdev,my_dev+i,1)<0)
		{
			printk("Error in cdev adding....\n");
			kobject_put(&(dev->cdev.kobj));
			if(i >= 1)
			{
				kfifo_free(&(dev->kfifo));
				kfree(dev);
				goto error; 
			}
			else
			{
				kfifo_free(&(dev->kfifo));
				kfree(dev);
				unregister_chrdev_region(my_dev,ndevices);
				return -EBUSY;
			}
		}
		printk("10: %d cdev_add\n", i);
		}
		printk(KERN_INFO "my : loaded\n");
		return retval;
		error:
		{	
			struct list_head *pos,*n;

			list_for_each_safe(pos,n,&dev_list)
			{
				dev = container_of(pos,C_DEV,list);
				kfree(dev);
			}
			unregister_chrdev_region(my_dev,ndevices);
		

        		return -ENOMEM; 
		}
}

static void __exit my_exit(void)
	
{
	struct list_head *pos,*n;

	list_for_each_safe(pos,n,&dev_list)
	{
		dev = container_of(pos,C_DEV,list);
		kfree(dev);
	}

		unregister_chrdev_region(my_dev,ndevices);
	
	printk("my : unloading\n");
}


module_init(my_init);
module_exit(my_exit);
