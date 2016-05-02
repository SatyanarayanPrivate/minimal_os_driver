#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/cdev.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/kfifo.h>

#define MAX_BUFFER_AREA 2048
#define HW_FIFO_SIZE 16
#define CUST_SERIAL_NO_PORTS 1 
#define NO_REG_ADDRESSES 8
#define CUST_SERIAL_NO_REGISTERS 8

typedef struct p_serial_dev {
				struct list_head list;
				struct cdev cdev;				
				unsigned long base_addr;
				int irq_no;
				int kfifo_size;
				struct kfifo write_kfifo;
				struct kfifo read_kfifo;
				spinlock_t spinlock_rd;
				spinlock_t spinlock_wr;
				unsigned char *read_buff;
				unsigned char *write_buff;
				wait_queue_head_t read_queue;
				wait_queue_head_t write_queue;
				//struct tasklet_struct tx;
				//struct tasklet_struct rx;
				}P_SERIAL_DEV;

P_SERIAL_DEV *my_dev;
LIST_HEAD (dev_list);

static struct class *pseudo_class;	/* pretend /sys/class */
static struct cdev *plp_kmem_cdev;	/* dynamically allocated at runtime. */
static dev_t plp_kmem_dev; 			// Storage for the first device no.
static struct cdev *plp_kmem_cdev;	/* dynamically allocated at runtime. */

// Function Declarations
irqreturn_t custom_serial_intr_handler (int irq_no, void *dev);

//------------------------------------------------------ OPEN --------------------------------------------------
static int plp_kmem_open(struct inode *inode, struct file *file)
{		
	P_SERIAL_DEV *dev;
	int ret;
	
	dev = kmalloc (sizeof (P_SERIAL_DEV), GFP_KERNEL);
	if (my_dev == NULL)
	{
		printk ("Error in kmalloc\n");
		return -ENOMEM;
	}
	
	// Initialise UART controller in normal mode
	dev->base_addr = 0x3f8;	// SET base address

	dev = container_of (inode->i_cdev, P_SERIAL_DEV, cdev);
	file->private_data = dev;
	printk(KERN_ALERT " IN OPEN MODE\n");

	dump_stack(); 
	
	// Enable interrupt operation in h/w controller
	outb (0x80, dev->base_addr + 3);	// LCR : bit-7: 1 for DLAB select
	outb (0x00, dev->base_addr + 1);	// DLM : baud rate higher byte
	outb (0x0c, dev->base_addr + 0);	// DLL : baud rate lower byte
	outb (0x03, dev->base_addr + 3);	// LCR : bit data; 1 stop bits ; no parity ;
	outb (0x01, dev->base_addr + 1);	// IER : Rx and Tx interrupt enable
	outb (0xc7, dev->base_addr + 2);	// FCR : interrupt for 14 byte, enable fifo


	// Install ISR of the device
	ret = request_irq (dev->irq_no, custom_serial_intr_handler, IRQF_DISABLED | IRQF_SHARED, "custom_serial_device0", dev);
	if(ret != 0)
	{
		printk(KERN_ALERT "Unable to Register Interrupt for Device\n");
		//must free resources allocated in open before this operation
		kfree (dev);
		return -EBUSY;
	}

	outb (0x08, dev->base_addr + 4);	// MCR : interrupt enable 

	#ifdef PLP_DEBUG
		printk(KERN_DEBUG "plp_kmem: opened device.\n");
	#endif
	return 0;  //success	
}
//------------------------------------------------------- ISR --------------------------------------------------
irqreturn_t custom_serial_intr_handler (int irq_no, P_SERIAL_DEV *dev)
{	
	int ret_val,i,j;
	irqreturn_t irq_r_flag=0;
	char byte, lk_buff[16];
	
	dev->base_addr = 0x3f8;
	irq_r_flag |= IRQ_NONE;

	// check the status bits in LSR	for TX HW FIFO empty 
	// if true, start transmitting certain no. of bytes to the tx HW FIFO
	if (inb(dev->base_addr+0x05 )& 0x20)
	{       
		//kfifo_get() is a non-blocking API that will 
		//read as much data as possible from the intermediate tx sw kfifo 

		ret_val = kfifo_out (&(dev->write_kfifo), lk_buff , HW_FIFO_SIZE); // Used instead of __kfifo_get ()
		// ret_val = kfifo_get (dev->write_kfifo, lk_buff, HW_FIFO_SIZE);
		if (ret_val!=0)
		{
			for (i=0; i<ret_val; i++)
		            outb (lk_buff[i], base_addr+0x0);
			
		}
		if ( kfifo_len (&(dev->write_kfifo)) == 0 )
		{
			outb (0x01, dev->base_addr + 1);	// IER : Disable Tx interrupt
		}
		irq_r_flag |= IRQ_HANDLED;
	}
	//here we are checking the rx HW fIFO status and reading
	//as much data that may be read into a local buffer
	if (inb(base_addr+0x05) & 0x01)
	{	
		for(j=0;;j++)  //this code is not the best way to write ??
		{
		    if (inb(base_addr+0x05) & 0x01)
		    {
				lk_buff[j] = inb(base_addr+0x0);
    		}
		    else 
				break;
  		}
		//here we dumping the data read into intermediate rx sw buffer
		kfifo_put (dev->read_kfifo, lk_buff,j);
		tasklet_schedule (&(dev->rx));
		//wake_up_interruptible (&(dev->read_queue));
  
		irq_r_flag |= IRQ_HANDLED; 
	}
	return rq_r_flag; 
}

//------------------------------------------------------ READ --------------------------------------------------
static ssize_t plp_kmem_read (struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int bytes;
	P_SERIAL_DEV *dev = file->private_data;

	printk ("In read method\n");

	bytes = kfifo_len (&(dev->read_kfifo));	
	if (bytes == 0)	// kfifo empty condition
	{
		if ( file->f_flags & O_NONBLOCK )
			return -EAGAIN;
		else	// wait if fifo is empty
			wait_event_interruptible ( dev->read_queue, (kfifo_len(&(dev->read_kfifo)) != 0 ) );
	}

	if ( access_ok (VERIFY_WRITE, (void __user*)buf, (unsigned long)count ) ) // Verifies that the respective user space buffer is leagaly in user space
		bytes = kfifo_out_locked (&(dev->read_kfifo),(char __user*)buf , bytes, &(my_dev->spinlock_rd) ); // Used instead of __kfifo_get ()
	else
		return -EFAULT;

	return bytes;
}
//------------------------------------------------------ WRITE -------------------------------------------------
static ssize_t plp_kmem_write (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct P_SERIAL_DEV *dev = file->private_data;  
	
	unsigned int i, ret_val, bytes;
	unsigned char ch;
	char lk_buff[16]; 
	
	printk (KERN_ALERT "** In write method **\n");
	
	if (kfifo_avail(&(dev->write_kfifo)) == 0) // kfifo full condition
	{
		if ( file->f_flags & O_NONBLOCK )
			return -EAGAIN;
		else    // wait if fifo is empty
			wait_event_interruptible ( dev->write_queue, (kfifo_avail(&(dev->write_kfifo)) != 0 ) );
	}
	utb (0x01, dev->base_addr + 1);	// IER : Enable Tx/Rx interrupt

	return 0;
}

//------------------------------------------------------ RELEASE -----------------------------------------------
static int plp_kmem_release(struct inode *inode, struct file *file)
{
	return 0;
}

//------------------------------------------------------ fops --------------------------------------------------
static struct file_operations plp_kmem_fops = 
		{
		.read	= plp_kmem_read,
		.write	= plp_kmem_write,
		.open 	= plp_kmem_open,
		.release= plp_kmem_release,
		.owner	= THIS_MODULE, 		//this is a must for the system ptr to this module's object/structure
		};

//------------------------------------------------------ INIT --------------------------------------------------
static int __init serial_init (void)
{
	int ret;
	struct resource *rs;
	
	my_dev = kmalloc (sizeof (P_SERIAL_DEV), GFP_KERNEL);
	if (my_dev == NULL)
	{
		printk ("Error in kmalloc\n");
		return -ENOMEM;
	}

	my_dev->base_addr = 0x3f8;	// SET base address
	
	// Gain exclusive access to the relevant I/O ports by request_region ()
	// request_region  (unsigned long start, unsigned long n, const char *name)
	rs = request_region (my_dev->base_addr, NO_REG_ADDRESSES, "custom_serial_device0");	
    	if(rs == NULL)
	{ 
		kfree(my_dev);
		return -EBUSY;
	}
	
	list_add_tail(&my_dev->list,&dev_list);

	// Allocate memory for write buffer
	my_dev->write_buff = kmalloc (MAX_BUFFER_AREA, GFP_KERNEL);
	if (my_dev->write_buff == NULL)
	{
		printk("error in write_buff's kmalloc()...\n");
		kfree(my_dev);
		// release_region  (unsigned long start, unsigned long n)
		// Release IO port region
		release_region (my_dev->base_addr, CUST_SERIAL_NO_PORTS);  
		return -ENOMEM;
	}

	// Allocate memory for read buffer
	my_dev->read_buff = kmalloc (MAX_BUFFER_AREA, GFP_KERNEL);
        if(my_dev->read_buff==NULL)
        {
		printk("error in read_buff's kmalloc()...\n");
		kfree(my_dev->write_buff);
		kfree(my_dev); 
		release_region(my_dev->base_addr,CUST_SERIAL_NO_PORTS);  
		return -ENOMEM;
	}

	spin_lock_init (&my_dev->spinlock_rd);
	spin_lock_init (&my_dev->spinlock_wr);
	
	// Initialise write kfifo
	// void kfifo_init (struct kfifo *fifo, void *buffer, unsigned int size);
	kfifo_init (&(my_dev->write_kfifo), my_dev->write_buff, MAX_BUFFER_AREA);
	if (&(my_dev->write_kfifo) == NULL)
	{
		printk ("Error in kfifo_init\n");
		kfree (my_dev->write_buff);
		kfree (my_dev->read_buff);
		kfree (my_dev);
	}

	// Initialise read kfifo
	kfifo_init (&(my_dev->read_kfifo), my_dev->read_buff, MAX_BUFFER_AREA);
	if (&(my_dev->read_kfifo) == NULL)
	{
		printk ("Error in kfifo_init\n");
		kfree (my_dev->write_buff);
		kfree (my_dev->read_buff);
		kfree (my_dev);
	}

	// Initialise wait queues
	init_waitqueue_head (&my_dev->read_queue);
	init_waitqueue_head (&my_dev->write_queue);

	// void tasklet_init (struct tasklet_struct * t, void(*func)(unsigned long), unsigned long data)  	
	// tasklet_init (&(my_dev->tx), tasklet_tx, &my_dev);
	// tasklet_init (&(my_dev->rx), taskelt_rx, &my_dev);

	// int alloc_chrdev_region (dev_t *, unsigned minor_no, unsigned device_id, const char *logical_name);	
	ret = alloc_chrdev_region (&plp_kmem_dev, 0, 5, "custom_serial_driver");
	if (ret == 0)
		goto error;
	
	cdev_init (&my_dev->cdev, &plp_kmem_fops);  
	kobject_set_name (&(my_dev->cdev.kobj), "custom_serial_dev0");
	my_dev->cdev.ops = &plp_kmem_fops; /* file up fops */
	
	if (cdev_add(&my_dev->cdev, plp_kmem_dev, 1)) 
	{	
		kobject_put(&(my_dev->cdev.kobj));
		unregister_chrdev_region(plp_kmem_dev, 1);
		kfree(my_dev);
		goto error;
	}

	pseudo_class = class_create (THIS_MODULE, "custom_serial_class");
	if (IS_ERR(pseudo_class)) 
	{
		printk(KERN_ERR "plp_kmem: Error creating class.\n");
		cdev_del (plp_kmem_cdev);
		unregister_chrdev_region(plp_kmem_dev, 1);
		//ADD MORE ERROR HANDLING
		goto error;
	}

	//device_create(pseudo_class, NULL, plp_kmem_dev, NULL, "pseudo_dev%d",i);
	device_create(pseudo_class, NULL, plp_kmem_dev, NULL, "custom_serial_dev0");
	
	printk(KERN_INFO "plp_kmem: loaded.\n");

	return 0;

error : 	
	printk(KERN_ERR "plp_kmem: cannot register device.\n");
	return -EINVAL;
	
}

//---------------------------------------------------- EXIT ----------------------------------------------------
static void __exit serial_exit (void)
{
	device_destroy (pseudo_class, plp_kmem_dev);
	class_destroy (pseudo_class);

	//removing the registration of my driverCUST_SERIAL_NO_REGISTERS
	cdev_del (&(my_dev->cdev));//remove the registration of the driver/device

	//freeing the logical resources - device nos.
	unregister_chrdev_region (plp_kmem_dev, 1);

	//freeing the system-space buffer
	kfree (my_dev); 
	release_region (my_dev->base_addr, CUST_SERIAL_NO_REGISTERS);
	printk (KERN_INFO "plp_kmem: unloading.\n");
}

//--------------------------------------------------------------------------------------------------------------

module_init (serial_init);
module_exit (serial_exit);

MODULE_DESCRIPTION("Demonstrate kernel memory allocation");

MODULE_ALIAS("memory_allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");
