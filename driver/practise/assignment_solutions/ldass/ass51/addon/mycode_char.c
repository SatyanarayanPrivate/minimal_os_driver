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

#define MAX_BUFFSIZE (2*1024)

static dev_t pcdd_dev;
//static struct cdev *pcdd_cdev;

static int __init pcdd_init(void); //prototypes
static void __exit pcdd_exit(void);

static int pcdd_open(struct inode *inode,struct file *file);
static int pcdd_release(struct inode *inode,struct file *file);
static ssize_t pcdd_read(struct file *file,char __user *buff,size_t count,loff_t *pos);
static ssize_t pcdd_write(struct file *file,const char __user *buff,size_t count,loff_t *pos);

typedef struct custom_obj1{
        struct kobject kobj;

}CUS_OBJ;


typedef struct priv_obj1
{
	struct list_head list;

        CUS_OBJ *custom_obj;
	dev_t dev;
	struct cdev cdev;
	unsigned char *buff;  //kernel buffer used with kfifo object
	struct kfifo kfifo;  //kfifo object ptr 
	spinlock_t my_lock;
	wait_queue_head_t queue; 
}C_DEV;


C_DEV *my_dev;
#define custom_obj(x) container_of(x,CUS_OBJ,kobj)


struct custom_attribute {
            struct attribute attr;
            ssize_t (*show)(CUS_OBJ *foo, struct custom_attribute *attr, char *buf);
	    ssize_t (*store)(CUS_OBJ *foo, struct custom_attribute *attr, const char *buf, size_t count);
};

#define custom_attr(x) container_of(x,struct custom_attribute,attr)

static ssize_t custom_attr_show(struct kobject *kobj,
			     struct attribute *attr,
			     char *buf)
{
	struct  custom_attribute *attribute;
	CUS_OBJ  *foo;

 	attribute = custom_attr(attr);
	foo = custom_obj(kobj);     
        //attribute=container_of(attr,struct custom_attribute,attr);
        //foo=container_of(kobj,CUS_OBJ,kobj);
   
	if (!attribute->show)
		return -EIO;

	return attribute->show(foo, attribute, buf);
}


static ssize_t custom_attr_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
	struct custom_attribute *attribute;
	CUS_OBJ *foo;

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
	CUS_OBJ *foo;

	foo = custom_obj(kobj);
	kfree(foo);
}


static ssize_t private_show(CUS_OBJ *my_obj, struct custom_attribute *attr,
		      char *buf)
{
	int bytes;
	C_DEV *priv_dev;
//	struct kobject *kobj;
//	kobj=container_of(my_obj, struct kobject, my_obj);
	priv_dev=container_of(&my_obj, C_DEV, custom_obj);
	if (strcmp(attr->attr.name, "resetdevice") == 0)
		{
			printk("inside attr reset.....\n");
			kfifo_reset(&my_dev->kfifo);
			return sprintf(buf, "kfifo reset....\n");;
		}
	else if (strcmp(attr->attr.name, "availablekfifo") == 0)
		{
			
			
			bytes = kfifo_size(&my_dev->kfifo) - kfifo_len(&my_dev->kfifo);
			printk("inside attr querry.....%d\n", bytes);
			return sprintf(buf, "available  = %d\n", bytes);
		}
	else
		{
			return sprintf(buf, "Major = %d  Minor = %d\n",MAJOR(my_dev->cdev.dev), MINOR(my_dev->cdev.dev));
		}
	
}

static ssize_t private_store(CUS_OBJ *foo_obj, struct custom_attribute *attr,
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



static struct file_operations pcdd_fops = {
	.open    = pcdd_open,
	.read    = pcdd_read,
	.write   = pcdd_write,
	.release = pcdd_release,
	.owner   = THIS_MODULE
};

LIST_HEAD(dev_list);
int ndevices=1;


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
        
        
	example_kset = kset_create_and_add("kset_devices_typeA", NULL, kernel_kobj);
	if (!example_kset)
		{
		unregister_chrdev_region(pcdd_dev,ndevices); 
		return -ENOMEM;
		}

    



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
			
		}
		printk("10: %d cdev_add\n", i);


                my_dev->custom_obj = kzalloc(sizeof(CUS_OBJ), GFP_KERNEL);

		if(!my_dev->custom_obj)
		{
			printk("ERROR:error in creating kobject.\n");
				kfifo_free(&(my_dev->kfifo));    ///????
			    	//kfree(my_dev->buff);
				kfree(my_dev);
				unregister_chrdev_region(pcdd_dev,ndevices);
				kset_unregister(example_kset);

			return -1;
		}

                my_dev->custom_obj->kobj.kset = example_kset;		//ksettttttttt
		printk("11: %d kset\n", i);
                int ret;
		ret = kobject_init_and_add(&my_dev->custom_obj->kobj, &custom_ktype, NULL, "Device%d", i);
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

	}
	printk(KERN_INFO "pcdd : loaded\n");
	return 0;
             
              error:
			{
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
		kfifo_free(&(p->kfifo));                               ///????
		//kfree(p->buff);
		kobject_put((p->custom_obj));
		cdev_del(&p->cdev);
		kfree(p);
	}
	printk("in exit list_for_each_entry_safe\n");
	unregister_chrdev_region(pcdd_dev,ndevices); 
	printk("in exit unregister_chrdev_region\n");
        kset_unregister(example_kset);
	printk("pcdd : unloading\n");
}




/*int i;
	for(i=0;i<ndevices;i++)
	{
		cdev_del(&(my_dev->cdev));
		kfifo_free(&(my_dev->kfifo));
		kfree(my_dev);
	}
	unregister_chrdev_region(pcdd_dev,ndevices);

  	printk("pcdd : unloading\n");
} */

module_init(pcdd_init);
module_exit(pcdd_exit);

MODULE_DESCRIPTION("Pseudo Device Driver");
MODULE_ALIAS("memory allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");



