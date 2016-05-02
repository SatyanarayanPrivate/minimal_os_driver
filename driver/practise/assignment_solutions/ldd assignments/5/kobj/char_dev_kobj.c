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


#define MAX_BUFFSIZE 1024
#define PLP_KMEM_BUFSIZE 1024

static int device_open(struct inode *inode,struct file *file);
static int device_release(struct inode *inode,struct file *file);
static ssize_t device_read(struct file *file,const char __user *buff,ssize_t count,loff_t *pos);
static ssize_t device_write(struct file *file,const char __user *buff,ssize_t count,loff_t *pos);

static dev_t pdevice;
//--------------------------------attributes----------------------------------------
static int major_no;
static int minor_no;
static int length;
//---------------------------------------------------------------------------------

static struct file_operations device_fops = {
	.open    = device_open,
	.read    = device_read,
	.write   = device_write,
	.release = device_release,
	.owner   = THIS_MODULE,
};


typedef struct priv_obj
{
	struct list_head list;
	dev_t dev;
	struct cdev cdev;
	struct kobject *my_kobj;  //kobject entry
	void *buff;
	struct kfifo kfifo;
	spinlock_t my_lock;
	wait_queue_head_t queue;
}c_dev;

//LIST_HEAD(dev_list);

c_dev *my_dev;

//--------------------------------function_to_show_and_store----------------------------------

static ssize_t atrb_show(struct kobject *kobj, struct kobj_attribute *attr,
		      char *buf)
{
	int var;

	sscanf(buf, "%du", &var);
	
	if (strcmp(attr->attr.name, "major_no") == 0)
		var = MAJOR(pdevice);
	else if (strcmp(attr->attr.name, "minor_no") == 0)	
		var = MINOR(pdevice);
	else
		var = kfifo_len(&(my_dev->kfifo));

	return sprintf(buf, "%d\n", var);
}

static ssize_t atrb_store(struct kobject *kobj, struct kobj_attribute *attr,
		       const char *buf, size_t count)
{
	int var;
       
	sscanf(buf, "%du", &var);
	
	if (strcmp(attr->attr.name, "major_no") == 0)
		 var=major_no ;
	else if (strcmp(attr->attr.name, "minor_no") == 0)
		var=minor_no ;
	else
		var=length;
	
	return count;
}
//--------------------------------------------------------------------------------------------

static struct kobj_attribute major_no_attribute = __ATTR(major_no, 0666, atrb_show, atrb_store);
static struct kobj_attribute minor_no_attribute = __ATTR(minor_no, 0666, atrb_show, atrb_store);
static struct kobj_attribute length_attribute = __ATTR(length, 0666, atrb_show, atrb_store);

static struct attribute *attrs[] = {
	&major_no_attribute.attr,
	&minor_no_attribute.attr,
	&length_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

/*--------------------------------------------------------------------------------------------
*struct kobj_attribute {
*        struct attribute attr;
*        ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr,
*                        char *buf);
*        ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr,
*                         const char *buf, size_t count);
*};
*/
//----------------------------open------------------------------------------------------------
static int device_open(struct inode *inode,struct file *file)
{
	c_dev *dev;
	dev = container_of(inode-> i_cdev ,c_dev, cdev);  //container_of(pointer,type,feild)  //"i_cdev" see <fs.h>
	file->private_data=dev;                           // i_cdev is member of struct cdev
	
	printk("Driver : opened device\n");
	return 0;
}

//--------------------------release------------------------------------------------------------
static int device_release(struct inode *inode,struct file *file)
{
	printk("Driver : closed device\n");
	return 0;
}

//--------------------------dev_read-----------------------------------------------------------
//syntax:
//unsigned int kfifo_out(struct kfifo *fifo, void *to, unsigned int len);
//unsigned long copy_to_user(void __user *to,const void *from,unsigned long count);

//access_ok -> verifies that the respective user space buffer is legally in user space

static ssize_t device_read(struct file *file,const char __user *buff,ssize_t count,loff_t *pos)
{
      
	c_dev *dev;
	unsigned int bytes;
	unsigned long ret;
	unsigned char lbuf[MAX_BUFFSIZE];
        dump_stack(); 
	dev = file->private_data;    // w.r.t to device setting file pointer 
	printk("Inside read call...\n");
        
        printk("length of count is calculated as %d\n",count);
	bytes = kfifo_len(&(dev->kfifo));   //returns the number of used bytes in the FIFO
	if(bytes==0)
	{            // for checking no. of queued bytes inside kfifo
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		
	}
	
	printk("length of bytes is calculated as %d\n",bytes);

	if(access_ok(VERIFY_WRITE,(void __user*)buff,(unsigned long)count))
	{
		printk("access is okkk\n");
		printk("In access ok ,length is calculated as %d\n",bytes);
		bytes = kfifo_out(&(dev->kfifo),lbuf,count);
           	if (bytes > 0)
			printk("bytes out from KFIFO is %d\n",bytes);
		else	
			printk("no data out\n");
      		printk("data is copied to local buffer from KFIFO successfully\n");		
		
	}
	else
		return -EFAULT;

        ret=copy_to_user((void __user*)buff,(unsigned char *)lbuf,bytes);       //instead of count we can write bytes
	printk("data is copied to user buffer from local buffer successfully\n");
	if(ret<0)
        	return -EFAULT;
	
	printk(KERN_INFO "exited from read method\n"); 
       	return bytes;
}

//-----------------------------------dev_write--------------------------------------------------------------
//syntax:
//unsigned long copy_from_user(void *to,const void __user *from,unsigned long count);
//unsigned int kfifo_in(struct kfifo *fifo, const void *from, unsigned int len);

static ssize_t device_write(struct file *file,const char __user *buff,ssize_t count,loff_t *pos)
{
	c_dev *dev;
	unsigned long ret,data;
        ssize_t byte = count;
        unsigned int bytes=0;
	unsigned char lbuf[MAX_BUFFSIZE];

	loff_t fpos = *pos;	

	dev = file->private_data;
	printk("Inside write call...\n");
        
	if (fpos >= PLP_KMEM_BUFSIZE)
		return -ENOSPC;

	if (fpos+byte >= PLP_KMEM_BUFSIZE)
		byte = PLP_KMEM_BUFSIZE-fpos;

	if (0 == (data = kmalloc(byte, GFP_KERNEL)))
		return -ENOMEM; 
     
       
        printk("length of count is calculated as %d\n",count);         
	/*if(access_ok(VERIFY_READ,(void __user*)buff,(unsigned long)count))
	{       
                ret = copy_from_user((void *)lbuf,(void __user *)buff,count); 
		
		printk("length of ret while copy_from_user is calculated as %d\n",count);               
		if(ret<0)
	                return -EFAULT; 
	
                
		 
                bytes = kfifo_in(&(dev->kfifo),(unsigned char *)lbuf,count);
		printk("data bytes used in write method is %d\n",bytes);
		return bytes;              
		
        }
         else
           return -EFAULT;
	*/

// data is writen to local buffer from user buffer and this will also verify that 
// the user-space buffer is really user-space buffer
 
	if (copy_from_user((void *)lbuf, (void __user *)buff, count)) 
	{
		printk(KERN_ERR "plp_kmem: cannot read data.\n");
		kfree(data);
		return -EFAULT;
	}

// data is write into the KFIFO by kfifo_in()
	
	bytes = kfifo_in(&(dev->kfifo),(unsigned char *)lbuf,count);
	
	pos = fpos+bytes;
        
	printk(KERN_INFO "exited from write method\n");
        return bytes;
         
}

//---------------------------------------------init_fun----------------------------------------------------------
//  void kfifo_init(struct kfifo *fifo, void *buffer, unsigned int size);
// extern struct kobject * __must_check kobject_create_and_add(const char *name, struct kobject *parent);
// static inline int sysfs_create_group(struct kobject *kobj,const struct attribute_group *grp);

static int __init pseudo_init(void)
{ 
        
        int i,retval=0;
	printk("we are in init function \n");
	
	
	i = alloc_chrdev_region(&pdevice,0,1,"pseudo");  //for ndevices..
        if(i>0)
        {
	         printk("Error in device creating.....\n");
	        return -EBUSY;
        } 
    
	my_dev=kmalloc(sizeof(c_dev),GFP_KERNEL);
	if(my_dev==NULL)
	{
	         printk("error in creating devices\n");
	         unregister_chrdev_region(pdevice,1);
        }
       
 //       list_add_tail(&my_dev->list,&dev_list);   --- does not require this list

        //---------------------------------------------------------------------
	my_dev -> my_kobj = kobject_create_and_add("kobject_example", kernel_kobj);
	if (!(my_dev -> my_kobj))
		return -ENOMEM;

	retval = sysfs_create_group(my_dev->my_kobj, &attr_group);
	if (retval)
		kobject_put(my_dev->my_kobj);

	//----------------------------------------------------------------------
       
	my_dev->buff = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);  // creating memory for buffer in kernel
	if(my_dev==NULL)
	{
		kfree(my_dev);
                unregister_chrdev_region(pdevice,1);
                return -ENOMEM;
        }
         
         kfifo_init(&(my_dev->kfifo),my_dev->buff,MAX_BUFFSIZE);   // ???
     	 if(&(my_dev->kfifo)==NULL)
         {
         	kfree(my_dev);
         	unregister_chrdev_region(pdevice,1);
         }
         
        cdev_init(&my_dev->cdev,&device_fops);         //device operations
      
         kobject_set_name(&(my_dev->cdev.kobj),"device0");//increment the reference count
        my_dev->cdev.ops = &device_fops;
         
        if(cdev_add(&my_dev->cdev,pdevice,1)<0)
          {
           printk("error in adding char device\n");
           kobject_put(&(my_dev->cdev.kobj));           // decrements the reference count &frees the object 
           kfifo_free(&my_dev->kfifo);
           //kfree(my_dev->buff);   //????

           kfree(my_dev);
           unregister_chrdev_region(pdevice,1);
           }
     
      printk(KERN_INFO "device has been loaded\n");
 	 
	return 0;


}

//-------------------------------------------------exit_fun--------------------------------------------------------
static void __exit pseudo_exit(void)
{
	printk("device unloaded\n");
        kfifo_free(&my_dev->kfifo);    
     //   kfree(my_dev->buff); 
        kfree(my_dev);
	unregister_chrdev_region(pdevice,1);
}

//-------------------------------------
module_init(pseudo_init);
module_exit(pseudo_exit);
	
MODULE_LICENSE("GPL");
