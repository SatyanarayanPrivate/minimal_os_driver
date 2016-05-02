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
#include<linux/proc_fs.h>
#include<linux/seq_file.h>

#define MAX_BUFFSIZE (8*1024)
//#define err 0



#define QUERRY 1010
#define RESET 2020

static dev_t pcdd_dev;
//static struct cdev *pcdd_cdev;

static int __init pcdd_init(void);
static void __exit pcdd_exit(void);

static int pcdd_open(struct inode *inode,struct file *file);
static int pcdd_release(struct inode *inode,struct file *file);
static ssize_t pcdd_read(struct file *file,const char __user *buff,ssize_t count,loff_t *pos);
static ssize_t pcdd_write(struct file *file,const char __user *buff,ssize_t count,loff_t *pos);
static int pcdd_ioctl(struct inode *inode,struct file *file, unsigned int command, unsigned long param);


typedef struct priv_obj1
{
	struct list_head list;
	struct kobject kobj;
	dev_t dev;
	struct cdev cdev;
	unsigned char *buff;  //kernel buffer used with kfifo object
	struct kfifo kfifo;  //kfifo object ptr ???? 
	spinlock_t my_lock;
	wait_queue_head_t queue; //you may need more than one wq 
}C_DEV;


#define custom_obj(x) container_of(x, C_DEV, kobj)

struct custom_attribute {
	struct attribute attr;
	ssize_t (*show)(C_DEV *foo, struct custom_attribute *attr, char *buf);
	ssize_t (*store)(C_DEV *foo, struct custom_attribute *attr, const char *buf, size_t count);
};
#define custom_attr(x) container_of(x, struct custom_attribute, attr)


static ssize_t custom_attr_show(struct kobject *kobj,
			     struct attribute *attr,
			     char *buf)
{
	struct custom_attribute *attribute;
	C_DEV *foo;

	attribute = custom_attr(attr);
	foo = custom_obj(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(foo, attribute, buf);
}

static ssize_t custom_attr_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
	struct custom_attribute *attribute;
	C_DEV *foo;

        //dump_stack();
	attribute = custom_attr(attr);
	foo = custom_obj(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(foo, attribute, buf, len);
}


static struct sysfs_ops custom_sysfs_ops = {
	.show = custom_attr_show,
	.store = custom_attr_store,
};


static void custom_release(struct kobject *kobj)
{
	C_DEV *foo;

	foo = custom_obj(kobj);
	kfree(foo);
}


static ssize_t private_show(C_DEV *my_obj, struct custom_attribute *attr,
		      char *buf)
{
	int bytes;
	if (strcmp(attr->attr.name, "resetdevice") == 0)
		{
			printk("inside attr reset.....\n");
			kfifo_reset(&my_obj->kfifo);
			return sprintf(buf, "kfifo reset....\n");
		}
	else if (strcmp(attr->attr.name, "availablekfifo") == 0)
		{
			printk("inside attr querry.....\n");
			
			bytes = kfifo_size(&my_obj->kfifo) - kfifo_len(&my_obj->kfifo);
			return sprintf(buf, "available  = %d\n", bytes);
		}
	else
		{
			return sprintf(buf, "Major = %d  Minor = %d\n",MAJOR(my_obj->cdev.dev), MINOR(my_obj->cdev.dev));
		}
	
}

static ssize_t private_store(C_DEV *foo_obj, struct custom_attribute *attr,
		       const char *buf, size_t count)
{
	
	if (strcmp(attr->attr.name, "resetdevice") == 0)
		return -1;
	else if (strcmp(attr->attr.name, "availablekfifo") == 0)
		return -1;
	else
		return -1;

}

static struct custom_attribute rst_attribute =
	__ATTR(resetdevice, 0666, private_show, private_store);
static struct custom_attribute availfifo_attribute =
	__ATTR(availablekfifo, 0666, private_show, private_store);
static struct custom_attribute majmin_attribute =
	__ATTR(majorminor, 0666, private_show, private_store);

static struct attribute *custom_default_attrs[] = {
	&rst_attribute.attr,
	&availfifo_attribute.attr,
	&majmin_attribute.attr,
	NULL,	
};



static struct kobj_type custom_ktype = {
	.sysfs_ops = &custom_sysfs_ops,
	.release = custom_release,
	.default_attrs = custom_default_attrs,
};


static struct kset *example_kset;
/////////////////////////////////////////


static struct file_operations pcdd_fops = {
	.open    = pcdd_open,
	.read    = pcdd_read,
	.ioctl   = pcdd_ioctl,
	.write   = pcdd_write,
	.release = pcdd_release,
	.owner   = THIS_MODULE,
};

LIST_HEAD(dev_list);
int ndevices=1;

static struct proc_dir_entry *entry = NULL ; 


static int pcdd_open(struct inode *inode,struct file *file)
{
        //must complete open method - ???

	C_DEV *obj;
	obj = container_of(inode->i_cdev, C_DEV, cdev);
	file->private_data = obj;

	printk("Driver : opend device\n");

	
	return 0;
}


static int pcdd_ioctl(struct inode *inode,struct file *file, unsigned int command, unsigned long param)
{
       


	C_DEV *dev;
	int bytes;
	
	printk("inside ioctl call...\n");

	dev = file->private_data;
	struct kfifo *kfifooperate = &dev->kfifo;

	switch (command)
	{
		case QUERRY:
		{
			printk("inside ioctl querry.....\n");
			struct kfifo *kfifooperate = &dev->kfifo;
			bytes = kfifo_len(kfifooperate);
			return bytes;
			break;
		
		}

		case RESET:
		{
			printk("inside ioctl reset.....\n");
			kfifo_reset(kfifooperate);
			return 0;
			break;

		}
	
		default:
			return -1;

	}


	
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
       
	dev = file->private_data;
	printk("inside read call...count %d........\n", count);

 
	struct kfifo *kfifooperate = &dev->kfifo;
	bytes = kfifo_len(kfifooperate);
	printk("inside read call...%d bytes from kfifo_len.... \n", bytes);
	if(bytes==0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{

		printk("inside read call...going to wait queue \n");
        wait_event_interruptible(dev->queue,(kfifo_len(kfifooperate) != 0));
		}
	}
        //access_ok() validates user space buffer ptr and its locations
        //if it is a valid user-space buffer, return 1 - otherwise, 0 

	if(access_ok(VERIFY_WRITE,(void __user*)buff,(unsigned long)count))
	{
		//bytes = __kfifo_get(&dev->kfifo,(unsigned char*)buff,count);

		bytes = kfifo_out(kfifooperate,(unsigned char*)buff,count);

		printk("inside read call...%d bytes of data read successfully.... \n", bytes);
		wake_up_interruptible(&dev->queue);
		return bytes;
	}
	else
		return -EFAULT;

}


static ssize_t pcdd_write(struct file *file,const char __user *buff,ssize_t count,loff_t *pos)
{
	C_DEV *dev;
	int bytes;
	dev = file->private_data;
	printk("inside write call... count %d..... \n", count);

	
	struct kfifo *kfifooperate = &dev->kfifo;
	

	bytes = kfifo_avail(kfifooperate);
	printk("inside write call...%d bytes from kfifo_avail.... \n", bytes);
        if(bytes == 0)
	{
		
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{

     

		printk("inside write call...going to wait queue \n");
        wait_event_interruptible(dev->queue,(kfifo_avail(kfifooperate) != 0));
		}
	}
     
	if(access_ok(VERIFY_READ,(void __user*)buff,(unsigned long)count))
	{
		

		bytes = kfifo_in(kfifooperate,(unsigned char*)buff,count);

		printk("inside write call...%d bytes of data written successfully.... \n", bytes);
		wake_up_interruptible(&dev->queue);
		return bytes;
	}
	else
		return -EFAULT;

         

}


static void *
mydrv_seq_start(struct seq_file *seq, loff_t *pos)
{
  C_DEV *p;
  loff_t off = 0;
  /* The iterator at the requested offset */
 
 

  list_for_each_entry(p, &dev_list, list) {
    if (*pos == off++) {
                        printk("in start : success %d\n",*pos);
                        return p;
    }
  }
  printk("in seq_start : over\n");
  return NULL;
}


static void *
mydrv_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
  struct list_head *n = ((C_DEV *)v)->list.next;
  ++*pos; 

   printk("in seq_next :%d\n",*pos);
   


  return(n != &dev_list) ?
          list_entry(n, C_DEV, list) : NULL;
}



static int
mydrv_seq_show(struct seq_file *seq, void *v)
{
   int ret, major, minor;
   C_DEV  *p =v;

   printk("in seq_show \n");
   
	major = (p->cdev.dev)/1048576;
	minor = (p->cdev.dev) - (1048576 * major);
   ret = seq_printf(seq,"Major no-> %d, Minor no-> %d\n", major, minor);
   printk(KERN_INFO "the return value of seq_printf is %d\n", ret); 

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
  
   printk("we are in proc mydrv_seq_open\n");   
 
 
   return seq_open(file, &mydrv_seq_ops);
}


static struct file_operations mydrv_proc_fops = {
   .owner    = THIS_MODULE,    
   .open     = mydrv_seq_open, 
   .read     = seq_read,         
   .llseek   = seq_lseek,         
   .release  = seq_release,     
};


C_DEV *my_dev;


char name[10]={"Dev0"};

module_param(ndevices,int,S_IRUGO);


static int __init pcdd_init(void)
{	
	int i, ret;
//	char name[10][10] = {"Device0", "Device1", "Device2", "Device3", "Device4", "Device5"};
	
	if(alloc_chrdev_region(&pcdd_dev,0,ndevices,"pseudo_driver"))	//creating device ids for each device & storing it into corresponding device pcdd_dev
	{
		printk("Error in device creating.....\n");
		//err |= EBUSY;
		return -EBUSY;
	}
	printk("1: alloc_chrdrv_region end\n");

	entry = create_proc_entry("readme", S_IRUSR, NULL);

	if (entry) {

		entry->proc_fops = &mydrv_proc_fops; 
	}
	else 
	{
		return -EINVAL;
	}
///	 /sys/kernel/kset_devices_mytypeA directory
	example_kset = kset_create_and_add("kset_devices_mytypeA", NULL, kernel_kobj);
	if (!example_kset)
		{
		unregister_chrdev_region(pcdd_dev,ndevices); 
		return -ENOMEM;
		}

		printk("11:  creaate fooobj\n");
		


	for(i=0;i<ndevices;i++)
	{
		my_dev = kzalloc(sizeof(C_DEV),GFP_KERNEL);			//create memory for pseduo devices
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
				kset_unregister(example_kset);
				return -ENOMEM;
			}
		}
		printk("2 %d: kmallic for my_dev\n", i);
/*		list_add_tail(&my_dev->list,&dev_list);				//you must add to the list after completing initialization successfully !!!
		printk("3 %d: list add tail\n", i);
*/		
		my_dev->buff = kzalloc(MAX_BUFFSIZE,GFP_KERNEL);
		if(my_dev->buff == NULL)
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
				kset_unregister(example_kset);
				return -ENOMEM;
			}	
		}
		printk("4 %d: kmalloc buffer\n", i);
		spin_lock_init(&(my_dev->my_lock));
		printk("5: %d spin_lock init\n", i);
		
		//my_dev->kfifo = kfifo_init(my_dev->buff, MAX_BUFFSIZE, GFP_KERNEL);
		printk("6: %d kfifo init going to alloc\n", i);
		 kfifo_init(&(my_dev->kfifo), (my_dev->buff), (unsigned int) MAX_BUFFSIZE);
		printk("6: %d kfifo init alloc\n", i);
		
		if(&(my_dev->kfifo) == NULL)
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
				kset_unregister(example_kset);
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
				kfifo_free(&(my_dev->kfifo));     //inappropriate 
			    	//kfree(my_dev->buff);              //only kfifo_free() is sufficient !!
				kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices);
				kset_unregister(example_kset);
				//err |= -EBUSY;
				goto error; 
			}
			else
			{
				kfifo_free(&(my_dev->kfifo));    ///????
			    	//kfree(my_dev->buff);
				kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices);
				kset_unregister(example_kset);
				return -EBUSY;
			}
		}

		init_waitqueue_head(&my_dev->queue);
		printk("10: %d init_waitqueue\n", i);



		my_dev->kobj.kset = example_kset;		//ksettttttttt
		printk("11: %d kset\n", i);

		ret = kobject_init_and_add(&my_dev->kobj, &custom_ktype, NULL, "Dev%d", i);
		if (ret) {

			printk("12: %d kobject init & add failed\n", i);
			kfifo_free(&(my_dev->kfifo));
			kfree(my_dev);
			unregister_chrdev_region(pcdd_dev,ndevices);
			kset_unregister(example_kset);
			return -1;
		}
		printk("12: %d kobject init & add\n", i);
	
		//kobject_uevent(&(my_dev->kobj), KOBJ_ADD);

		list_add_tail(&my_dev->list,&dev_list);				//you must add to the list after completing 												initialization successfully !!!
		printk("3 %d: list add tail\n", i);
		printk("10: %d cdev_add\n", i);

	}
	printk(KERN_INFO "pcdd : loaded\n");
	return 0;

	error:
	{
                //must complete error handling 
		unregister_chrdev_region(pcdd_dev,ndevices); 
		kset_unregister(example_kset);
		C_DEV *p, *n;
		list_for_each_entry_safe(p,n, &dev_list, list)
		{
			kfifo_free(&(p->kfifo));                          /////????
			//kfree(p->buff);
			kfree(p);
		}


		return -ENOMEM; //use a variable to set the error code
						//return the same variable from each point of error 
	}
}

static void __exit pcdd_exit(void)
{
	

	

	C_DEV *p,*n;
  //	list_first_entry(my_dev, &dev_list, list)
	printk("in exit \n");
	list_for_each_entry_safe(p,n, &dev_list, list)               //what happened to cdev_del() ???
	{

                cdev_del(&p->cdev);
		kfifo_free(&(p->kfifo));                               ///????
		//kfree(p->buff);
		kobject_put(&(p->kobj));
	
		//kfree(p);
	}
	printk("in exit list_for_each_entry_safe\n");
	unregister_chrdev_region(pcdd_dev,ndevices); 
	printk("in exit unregister_chrdev_region\n");
        kset_unregister(example_kset);
	printk("pcdd : unloading\n");
}

module_init(pcdd_init);
module_exit(pcdd_exit);

MODULE_DESCRIPTION("Pseudo Device Driver");
MODULE_ALIAS("memory allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");



