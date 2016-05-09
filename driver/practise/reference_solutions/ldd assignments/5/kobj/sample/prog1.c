#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/list.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/vmalloc.h>
#include<linux/major.h>
#include<linux/cdev.h>
#include<linux/blkdev.h>
#include<asm/uaccess.h>
#include<linux/fcntl.h>
#include<linux/slab.h>
#include<linux/kfifo.h>
#include<linux/wait.h>
#include<linux/interrupt.h>
#include<linux/sysfs.h>
#include<linux/kobject.h>

#define kmem_buffer_size (4*2048)

//static char kmem_buffer[kmem_buffer_size];
int ndevices = 1;
int j=2;
static dev_t kmem_dev;
//static struct cdev *kmem_cdev;
static int __init prog_init(void);
static void __exit prog_exit(void);

static int my_fops_open(struct inode *inode,struct file *file);
static size_t my_fops_read(struct file *file,char __user *buf,size_t count, loff_t *ppos);
static size_t my_fops_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);

//================= private object =============================
typedef struct priv_obj
{
	struct list_head list;
	struct cdev cdev;
	unsigned char *buff;
	struct kfifo fifo;
	wait_queue_head_t queue;
	spinlock_t my_lock;
	struct kobject *kobj;
	
}P_OBJ1,C_DEV;

static int maj;
static int min;
static int bytes;
static int reset;
static int tmp=1;

C_DEV *my_cdev;
module_param(ndevices,int,0);
static LIST_HEAD(mylist);

//====================char device function =====================================

static int my_fops_open(struct inode *inode,struct file *file)
{	
	printk("------- OPEN -------\n");
	P_OBJ1 *obj;
	obj = container_of(inode -> i_cdev,P_OBJ1,cdev);
	file -> private_data  = obj;

	return 0;
}

static size_t my_fops_read(struct file *file, char __user *buf,size_t count, loff_t *ppos)
{	
	printk("------- READ -------\n");
	
	C_DEV *dev;
	int bytes = count;
	int ret,*ret5;
	dev = file -> private_data;
	printk("inside read call...\n");
	bytes = kfifo_len(&(dev->fifo));
	printk("bytes : %d\n",bytes);
	ret = kfifo_is_empty(&(dev -> fifo));
	printk("1.read---kfifo empty return value : %d\n",ret);
	
	ret = kfifo_is_full(&(dev -> fifo));
	printk("2.read---kfifo full return value : %d\n",ret);
	
	if(bytes==0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{	dump_stack();
			/*ret4 = dev -> queue;
			printk("ret4 : %d\n",ret4);
			*/
			ret5 = &(dev -> queue);
			printk("ret5 : %p\n",ret5);
			
			wait_event_interruptible((dev->queue),(kfifo_len(&(dev->fifo)) != 0));
			printk("1111");
		}
	}
	bytes = count;
	if(access_ok(VERIFY_WRITE,(void __user*)buf,(unsigned long)bytes))
	{	printk("1.bytes : %d\n",bytes);
		bytes = kfifo_out(&(dev->fifo),(char __user *)buf,bytes);
		printk("2.bytes : %d\n",bytes);
	}
	else
		return -EFAULT;
	
	/*if(i==0)
		{bytes = kfifo_out_peek(&(dev -> fifo),(char __user *)buf,bytes,0);
		i++;}
	else
	{
	bytes = kfifo_out(&(dev -> fifo),(char __user *)buf,bytes);
	}*/
	ret = kfifo_is_empty(&(dev -> fifo));
	printk("3.read---kfifo empty return value : %d\n",ret);
	
	ret = kfifo_is_full(&(dev -> fifo));
	printk("4.read---kfifo full return value : %d\n",ret);
	return bytes;
}
	
	

static size_t my_fops_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{	
	int bytes = count;
	int ret;
	printk("------- WRITE -------\n");
	//C_DEV *obj = file -> private_data;
	P_OBJ1 *dev;
	dev = file -> private_data;
	
	ret = kfifo_is_empty(&(dev -> fifo));
	printk("1. write----kfifo empty return value : %d\n",ret);
	
	ret = kfifo_is_full(&(dev -> fifo));
	printk("2. write----kfifo full return value : %d\n",ret);
	
	bytes = kfifo_in(&(dev -> fifo),(char __user *)buf,bytes);
	
	
	ret = kfifo_is_empty(&(dev -> fifo));
	printk("3. write----kfifo empty return value : %d\n",ret);
	
	ret = kfifo_is_full(&(dev -> fifo));
	printk("4. write----kfifo full return value : %d\n",ret);
	j=0;
	wake_up_all(&dev->queue);
	return bytes;
}

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = my_fops_open,
	.read = my_fops_read,
	.write = my_fops_write,	
};

//====================== private kobject show,store ================================= 

static ssize_t maj_min_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	printk("------ maj_min_show -------\n");
	return sprintf(buf,"Major: %d...Minor: %d\n", maj,min);
}

static ssize_t maj_min_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,size_t count)
{	printk("------ maj_min_store -------\n");
	maj = MAJOR(kmem_dev);
	min = MINOR(kmem_dev);
	sscanf(buf, "%d",tmp);
	return count;
}



static ssize_t bytes_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	printk("------ bytes_show -------\n");
	return sprintf(buf,"No. of bytes freed: %d\n", bytes);
	
}

static ssize_t bytes_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,size_t count)
{	printk("------ bytes_store -------\n");
	bytes = kfifo_size(&(my_cdev -> fifo)) - kfifo_len(&(my_cdev -> fifo));
	sscanf(buf, "%d", &bytes);
	return count;
}



static ssize_t reset_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	printk("------ reset_show -------\n");
	return sprintf(buf,"%d\n", reset);
}

static ssize_t reset_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,size_t count)
{	printk("------ reset_store -------\n");
	reset =1;
	kfifo_reset(&(my_cdev -> fifo));
	sscanf(buf, "%d", &reset);
	printk("successfully Reset\n");
	return count;
}

static struct kobj_attribute bytes_attribute = __ATTR(bytes,0666,bytes_show,bytes_store);
static struct kobj_attribute maj_min_attribute = __ATTR(maj_min,0666,maj_min_show,maj_min_store);
static struct kobj_attribute reset_attribute = __ATTR(reset,0666,reset_show,reset_store);

static struct attribute *attrs[] = {
	&maj_min_attribute.attr,
	&bytes_attribute.attr,
	&reset_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

//============================== INIT Module =======================================

static int __init prog_init(void)
{	
	dump_stack();
	printk("---------INIT MODULE--------\n");
	int ret,size,i;
	
	
	ret = alloc_chrdev_region(&kmem_dev,0,ndevices,"pseudo");
	printk("ndevices : %d\n",ndevices);
	printk("1.-- Return value of alloc_chrdev_region : %d\n",ret);
	printk("2.-- Device id : %lu\n", (unsigned long)kmem_dev);
	printk("Major No. : %lu\t Minor No. : %lu\n",(unsigned long)MAJOR(kmem_dev),(unsigned long)MINOR(kmem_dev));
	
	printk("Major No. : %lu\t Minor No. : %lu\n",(unsigned long)MAJOR(kmem_dev),(unsigned long)MINOR(kmem_dev));
		my_cdev = kmalloc(sizeof(C_DEV),GFP_KERNEL);
		if(!my_cdev)
		{
			printk("3.-- Error in creating malloc of C_DEV\n");
			if(i>1)
			{
				goto error;
			}
			else
			{
				unregister_chrdev_region(kmem_dev,ndevices);
				goto error;
			}
		}
		printk("4.-- Allocated the size of C_DEV for device %d\n",i);
		list_add_tail(&my_cdev -> list,&mylist);
		printk("5.-- Added the list in my_cdev for device %d\n",i);
		
		my_cdev -> buff = kmalloc(kmem_buffer_size,GFP_KERNEL);
		if(!my_cdev)
		{
			printk("6.-- Error in Allocating the memory to char device\n");
			if(i>1)
			{
				kfree(my_cdev);
				goto error;
			}
			else
			{
				kfree(my_cdev);
				unregister_chrdev_region(kmem_dev,ndevices);
				goto error;
			}
		}
		printk("7.-- Successful in Allocating the memory to char device\n");
		cdev_init(&my_cdev -> cdev, &my_fops);
		printk("8.-- Successful in Initailizing the cdev\n");
		spin_lock_init(&(my_cdev->my_lock));
		printk("9.-- Successful in Initailizing the Spin lock\n");
		init_waitqueue_head(&my_cdev -> queue);
		printk("10.-- Successful in Initailizing the waitqueue head\n");
			//kfifo_init((my_cdev -> kfifo), (my_cdev -> buff),kmem_buffer_size);
			//dump_stack();
		ret = kfifo_alloc(&(my_cdev->fifo),kmem_buffer_size,GFP_KERNEL);
		printk("11.-- Successful in Allocating the kfifo\n");
			//dump_stack();
		printk("12.-- ret value of kfifo_alloc : %d\n",ret);
		if(ret != NULL)
		{
			printk("12.-- Error in Allocating the kfifo\n");
			if(i>1)
			{
				kfree(my_cdev);
				kfree(my_cdev -> buff);
				goto error;
			}
			else
			{
				kfree(my_cdev);
				kfree(my_cdev -> buff);
				unregister_chrdev_region(kmem_dev,ndevices);
				goto error;
			}
		}
	untitled folder
	
	size = kfifo_size(&(my_cdev -> fifo));
	printk("13.-- size of kfifo : %d\n",size);

	//ret = kobject_set_name(&(my_cdev -> cdev.kobj),"psuedo_kobj");
	my_cdev -> kobj = kobject_create_and_add("my_kobject",kernel_kobj);
	if(!(my_cdev -> kobj))
	{	
		printk("Error in creating kobject\n");
		return -ENOMEM;
	}
	
	ret = sysfs_create_group(my_cdev -> kobj,&attr_group);
	printk("sysfs_create_group return value : %d\n",ret);
	if(ret)
		kobject_put(my_cdev -> kobj);
	printk("14.-- Return value of sysfs_create_group : %d\n\n",ret);

	if(cdev_add(&my_cdev -> cdev,kmem_dev,ndevices))
	{
		//kobject_put(&(my_cdev -> cdev.kobj));
		unregister_chrdev_region(kmem_dev,ndevices);
		kfree(my_cdev);
	}
	
	return 0;
	error:
	{
		return -ENOMEM;
	}
}

//========================= EXIT Module ================================================

static void __exit prog_exit(void)
{	
	printk("---------EXIT MODULE--------\n");
	int i;
	kobject_put((my_cdev -> kobj));
	kobject_del((my_cdev -> kobj));
	sysfs_remove_group(my_cdev -> kobj,&attr_group);
	kfifo_free(&(my_cdev -> fifo));
	printk("kfifo is freed\n");
	cdev_del(&(my_cdev -> cdev));
	kfree(my_cdev -> buff);
	kfree(my_cdev);
	unregister_chrdev_region(kmem_dev,ndevices);
	printk("Character device deleted\n");
	printk("Unregister the chardev region\n");
}

module_init(prog_init);
module_exit(prog_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Demonstrate cdev");

