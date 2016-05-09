#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/kfifo.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>
#include <linux/kdev_t.h>
#include <linux/list.h>


int  ndevices = 5;	// Number of pseudo devices
#define MAX_BUF (1024)

// Gloabl variables
static dev_t dev;	// Holds device number
struct cdev *i_cdev;	// inode for device

// Function declarations
static int dev_open (struct inode *inode, struct file *file);
static int dev_release (struct inode *inode, struct file *file);
static ssize_t dev_read (struct file *file, const char __user *buff, ssize_t count,loff_t *pos);
static ssize_t dev_write (struct file *file, const char __user *buff, ssize_t count,loff_t *pos);

typedef struct priv_obj {
			struct list_head list;
			struct cdev cdev_d;
			void *buf;
			struct kfifo kfifo;				
			spinlock_t my_lock;
			wait_queue_head_t queue1, queue2;
			}C_DEV;

C_DEV *my_dev;
LIST_HEAD (dev_list);	// Initialising list

static struct file_operations dev_fops = {
	       			.open = dev_open,
					.read = dev_read,
					.write = dev_write,
					.release = dev_release,
					.owner =THIS_MODULE,
					};

//------------------------------------------ OPEN ---------------------------------------------------------------
static int dev_open ( struct inode *i_dev, struct file *file )
{	
	C_DEV *pdev;
	pdev = container_of ( i_dev -> i_cdev, C_DEV, cdev_d ); 
	file -> private_data = pdev ;
	printk("Device is opened\n");
	return 0;
}
//------------------------------------------ RELEASE -------------------------------------------------------------
static int dev_release ( struct inode *inode, struct file *file )
{
	printk("Device resources are released\n");
	return 0;
}
//-------------------------------------------- READ ---------------------------------------------------------------
static ssize_t dev_read (struct file *file, const char __user *buff, ssize_t count,loff_t *pos)
{
	C_DEV *pdev;
	int bytes;
	int ret;
	char lbuf[MAX_BUF];

	pdev = file->private_data;
	printk("** Inside read method **\n");

	bytes = kfifo_len (&(pdev->kfifo));
	if (bytes == 0)	// kfifo empty condition
	{
		if ( file->f_flags & O_NONBLOCK )
			return -EAGAIN;
		else	// wait if fifo is empty
			wait_event_interruptible ( pdev->queue1, (kfifo_len(&(pdev->kfifo)) != 0 ) );
	}

	if ( access_ok(VERIFY_WRITE, (void __user*)buff, (unsigned long)count ) ) // Verifies that the respective user space buffer is leagaly in user space
		bytes = kfifo_out_locked (&(pdev->kfifo),(char __user*)buff , bytes, &(my_dev->my_lock) ); // Used instead of __kfifo_get ()
	else
		return -EFAULT;

	//unsigned long copy_to_user ( void __user *to, const void *from, unsigned long count );
	//ret = copy_to_user ( (char __user*)buff, lbuf, count );
	//if ( ret < 0 )
	//	return -EFAULT;

	printk (KERN_ALERT "Exited from read method\n");
	return bytes;
}

//---------------------------------------------- WRITE -------------------------------------------------------------
static ssize_t dev_write (struct file *file, const char __user *buff, ssize_t count, loff_t *pos)
{
	int bytes = count;
	int ret;
	C_DEV *pdev;
	char lbuf[MAX_BUF];
	//loff_t fpos = *pos;

	printk (KERN_ALERT "** In write method **\n");
	pdev = file->private_data;		

	if (kfifo_avail(&(pdev->kfifo)) == 0) // kfifo full condition
	{
		if ( file->f_flags & O_NONBLOCK )
			return -EAGAIN;
		else    // wait if fifo is empty
			wait_event_interruptible ( pdev->queue2, (kfifo_avail(&(pdev->kfifo)) != 0 ) );
	}


	if ( access_ok(VERIFY_READ, (void __user*)buff, (unsigned long)count ) ) // Verifies that the respective user space buffer is leagaly in user space
	{
		printk (KERN_ALERT "After acces_ok\n");
		//unsigned long copy_from_user ( void __user *to, const void *from, unsigned long count );
	//	ret = copy_from_user (lbuf, (unsigned char *)buff, count );
	//	if ( ret < 0 )
	//		return -EFAULT;
		printk (KERN_ALERT "After copy_from_user\n");
	
		// extern unsigned int kfifo_in(struct kfifo *fifo, const void *from, unsigned int len);
		bytes = kfifo_in_locked ( &(pdev->kfifo), (unsigned char *)buff, count, &(my_dev->my_lock));
		printk (KERN_ALERT "After kfifo_in\n");
	}
	else
		return -EFAULT;

	printk (KERN_ALERT "Exited write method. No of bytes written %d\n", bytes);
	return bytes;
}

//------------------------------------------------- INIT -----------------------------------------------------------
static int __init pseudo_init ( void )
{
	int ret, i; 
	struct list_head *cursor, *next;
	// int alloc_chrdev_region ( dev_t *dev, unsigned int firstminor, unsigned int count, char *name );
	ret = alloc_chrdev_region ( &dev, 0, ndevices, "pseudo_driver" );	
	if ( ret != 0 )
	{
		printk ( "Error in creating device\n");
		return -EBUSY;
 	}

	for (i=0; i<ndevices; i++) //------------------------------------------------------------------------------
	{
		my_dev = kmalloc ( sizeof ( C_DEV ), GFP_KERNEL );	// Allocate memory of size of C_DEV for our device 
		if ( my_dev == NULL )
		{
			printk ("Error in creating devices\n");
			if (i>=1)
			{
				goto error;
			}
			else
			{
				unregister_chrdev_region (dev, ndevices);
				return -ENOMEM;
			}
		}
		// list_add_tail (struct list_head *new, struct list_head *head);
		// Adds new entry immediately after the list head at the begining of the list
		list_add_tail (&(my_dev->list), &(dev_list));
		printk ("After list_add_tail for device%d\n", i);
	
		my_dev -> buf = kmalloc( MAX_BUF, GFP_KERNEL);		// Allocating memory for kfifo. kfifo uses my_dev->buf.
		if ( my_dev == NULL )
		{
			printk ("Error in kmalloc\n");
			if (i>=1)
			{
				kfree ( my_dev );
				goto error;
			}
			else
			{
				kfree ( my_dev );
			 	unregister_chrdev_region ( dev, ndevices );
				return -ENOMEM;
			}
		}
		printk ("After kmalloc for device%d\n", i);
		
		spin_lock_init (&(my_dev->my_lock));	
		printk ("After spin_lock_init %d\n", i);
		
		init_waitqueue_head (&(my_dev->queue1));
	    init_waitqueue_head (&(my_dev->queue2));

		// void kfifo_init (struct kfifo *fifo, void *buffer, unsigned int size);
		kfifo_init ( &(my_dev -> kfifo), my_dev -> buf, MAX_BUF );	// Newer	
		if ( &(my_dev -> kfifo) == NULL )
		{
			printk ("Error in kfifo\n");
			if (i>=1)
			{
				kfree (my_dev);
				unregister_chrdev_region (dev, ndevices);
				goto error;
			}
			else
			{
				kfree ( my_dev );
				unregister_chrdev_region ( dev, ndevices );
				return -ENOMEM;
			}
		}
		printk ("After kfifo_int for device%d\n", i);
	
		// void cedev_init ( struct cdev *cdev, struct file_operations *fops );
		cdev_init ( &(my_dev -> cdev_d), &(dev_fops) );	// Initialising my_dev
		
		kobject_set_name ( &( my_dev -> cdev_d.kobj), "my_device%d", i );

		my_dev -> cdev_d.ops = &dev_fops;
	
		ret =  cdev_add( &(my_dev->cdev_d), dev+i, ndevices ); // Driver is ready for use. "Live"	
		if ( ret < 0 )
		{
			printk("Not able to add device.\n");
			if (i>=1)
			{
				kobject_put( &(my_dev -> cdev_d.kobj) );	//?
				//kfifo_free (&(my_dev->kfifo));
				// kfree (my_dev->buf); No need. May freeze the system after some time
				//kfree (my_dev);
		 		//unregister_chrdev_region ( dev, ndevices );
				goto error;
			}
			else
			{
				kobject_put( &(my_dev -> cdev_d.kobj) );	//?
				kfifo_free (&(my_dev->kfifo));
				// kfree (my_dev->buf); No need. May freeze the system after some time
				kfree (my_dev);
		 		unregister_chrdev_region ( dev, ndevices );
				return -EBUSY;
			}
		}
		printk ("After cdev_add for device%d\n", i);
		printk("pseudo_driver is loaded: Mj = %d, Mi = %d\n",MAJOR(dev+i), MINOR(dev+i) );
		printk("pseudo_driver id: dev = %x\n", dev );
	} // End for loop -----------------------------------------------------------------------------------------
	
	return 0;

	error:
	{
		// list_for_each_safe (struct list_head *cursor, struct list_head *next, struct list_head *list);			
		list_for_each_safe (cursor, next, &(my_dev->list))
		{
			kfifo_free (&(my_dev->kfifo));
			kfree (my_dev);
		}		
	 	unregister_chrdev_region ( dev, ndevices );				
		return -ENOMEM;

	}
}
//----------------------------------------------- EXIT ------------------------------------------------------------

static void __exit pseudo_exit ( void )
{
	// void unregister_chrdev_region ( dev_t dev, unsigned int count );
	// extern void kfifo_free(struct kfifo *fifo);
	kfifo_free (&(my_dev->kfifo));
	kfree (my_dev);
 	unregister_chrdev_region ( dev, ndevices );	
	printk("Device is removed\n");
}
//-----------------------------------------------------------------------------------------------------------------

module_init ( pseudo_init );
module_exit ( pseudo_exit );

MODULE_LICENSE ( "GPL" );
