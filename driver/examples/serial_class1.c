#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/version.h>
#include<linux/device.h>
#include<linux/ioport.h>
#include<linux/pci.h>
#include<linux/slab.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<linux/wait.h>
#include<linux/kfifo.h>
#include<linux/timer.h>
#include<linux/jiffies.h>
#include<linux/sched.h>
#include<linux/string.h>
#include<linux/list.h>
#include<linux/errno.h>
#include<linux/device.h>
#include<linux/sysfs.h>
#include<linux/cdev.h>
#include<asm/unistd.h>
#include<asm/uaccess.h>
#include<asm/io.h>
#include<asm/fcntl.h>
#include<asm/errno.h>
#include<asm/ioctl.h>


#define SERIAL_NO_PORTS 8
#define HW_BUFF_SIZE 16
#define LOCAL_BUFF_SIZE 128

static int __init serial_init(void);
static void __exit serial_exit(void);
static int serial_open(struct inode *inode,struct file *file);
static int serial_release(struct inode *inode,struct file *file);
static ssize_t serial_read(struct file *file,char __user *buf,size_t count, loff_t *ppos);
static ssize_t serial_write(struct file *file,char __user *buf,size_t count, loff_t *ppos);

static struct class* serial_class;
static struct class_device* device;

ssize_t show_tx(struct class_device* device,char* buff);
ssize_t show_rx(struct class_device* device,char* buff);
ssize_t show_base_addr(struct class_device* device,char* buff);
ssize_t show_tx_poll_int(struct class_device* device,char* buff);
ssize_t show_rx_poll_int(struct class_device* device,char* buff);

CLASS_DEVICE_ATTR(tx,S_IRUGO,show_tx,NULL);
CLASS_DEVICE_ATTR(rx,S_IRUGO,show_rx,NULL);
CLASS_DEVICE_ATTR(base_addr,S_IRUGO,show_base_addr,NULL);
CLASS_DEVICE_ATTR(transmitter_poll_int,S_IRUGO,show_tx_poll_int,NULL);
CLASS_DEVICE_ATTR(receiver_poll_int,S_IRUGO,show_rx_poll_int,NULL);


static dev_t serial_dev;

typedef struct serial_dev
{
	struct cdev cdev;
	int base_addr;
	int kfifo_size;
	int poll_int_tx;
	int poll_int_rx;
	int num_tx;
	int num_rx;
	int num_drop;
	unsigned char *read_buff;
	unsigned char *write_buff;
	struct kfifo *read_kfifo; 
	struct kfifo *write_kfifo; 
	spinlock_t spinlock_rd;
	spinlock_t spinlock_wr;
	spinlock_t lock;
	wait_queue_head_t read_queue;
	wait_queue_head_t write_queue;
	struct timer_list tx_timer; //a s/w timer for tx data polling 
	struct timer_list rx_timer; //another s/w timer for rx data polling

}S_DEV;

static struct file_operations serial_fops = {
	.open    = serial_open,
	.release = serial_release,
	.read    = serial_read,
	.write   = serial_write,
	.owner   = THIS_MODULE,
};

ssize_t show_tx(struct class_device* device,char* buff)
{
	S_DEV *dev;
	dev = device->class_data;
	return snprintf(buff,PAGE_SIZE,"%d %s\n",dev->num_tx,"bytes transmitted");
}
ssize_t show_rx(struct class_device* device,char* buff)
{
	S_DEV *dev;
	dev = device->class_data;
	return snprintf(buff,PAGE_SIZE,"%d %s\n",dev->num_rx,"bytes received");
}
ssize_t show_base_addr(struct class_device* device,char* buff)
{
	S_DEV *dev;
	dev = device->class_data;
	return snprintf(buff,PAGE_SIZE,"%d %s\n",dev->base_addr,"ioport address");
}
ssize_t show_tx_poll_int(struct class_device* device,char* buff)
{
	S_DEV *dev;
	dev = device->class_data;
	return snprintf(buff,PAGE_SIZE,"%d %s\n",dev->poll_int_tx,"tx polling interval");
}
ssize_t show_rx_poll_int(struct class_device* device,char* buff)
{
	S_DEV *dev;
	dev = device->class_data;
	return snprintf(buff,PAGE_SIZE,"%d %s\n",dev->poll_int_rx,"rx polling interval");
}

static int serial_open(struct inode *inode,struct file *file)
{
	S_DEV *dev;
	dev = container_of(inode->i_cdev,S_DEV,cdev);
	file->private_data = dev;
	dev->num_drop=0;
	
	//setting DLAB bit of LCR reg
	outb(0x80,dev->base_addr + 0x03);
	//Inside DLAB setting divisor value 12 for baudrate 9600
	outb(0x00,dev->base_addr + 0x01);	//higher byte of DLAB i.e. DLM 
	outb(0x0c,dev->base_addr + 0x00);	//lower byte of DLAB i.e DLL
	//reset DLAB bit & setting stop bit = 1 & with no priority in LCR reg 
	outb(0x03,dev->base_addr + 0x03);
	//disabling the interrupt using IER reg
	outb(0x00,dev->base_addr + 0x01);
	//enabling the fifos for self clearing in FCR reg
	outb(0x07,dev->base_addr + 0x02);
	//enabling normal mode of UART in MCR
	outb(0x00,dev->base_addr + 0x04);

        //using mod_timer(), we are activating/triggering 
        //rx s/w timer, in open() method - time stamp value
        //is setup based on polling requirements of the
        //serial controller - for instance, it depends on the
        //current baud-rate of the serial controller !!! 
	
	mod_timer(&(dev->rx_timer),jiffies + dev->poll_int_rx);
	
	printk("driver : Device opened\n");
	return 0;
}


static ssize_t serial_read(struct file *file,char __user *buf,size_t count, loff_t *ppos)
{
	S_DEV *dev;
	int bytes;
	dev = file->private_data;
	bytes = kfifo_len(dev->read_kfifo);
	printk("we are entering read method\n");
	
	if(bytes==0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
			printk("we are in read wait queue\n");
			wait_event_interruptible(dev->read_queue,(kfifo_len(dev->read_kfifo)!=0));
		}
	}
	if(access_ok(VERIFY_WRITE,(void __user *) buf, (unsigned long) count))
	{
		bytes = kfifo_get(dev->read_kfifo,(void __user*)buf,(unsigned long)count);
		printk("we have successfuly read %d bytes\n", bytes); 
		return bytes;
	}
	else
		return -EFAULT;

}

//rx_handler is invoked, when the current rx s/w timer's time-stamp
//is expired 
//
//in our context, rx_handler() copies data from hw rx buffer and
//copies it into rx kfifo's buffer !!!
//
//in addition, setup another s/w timer with future time-stamp for
//next polling event 
//
//in addition, wake-up any process(es) blocked in wq of the 
//rx part of the driver/device !!!
//
//similarly, there must be a tx polling timer / handler - however, 
//polling frequency of tx s/w timer will be different from 
//rx timer / handler !!!


//=============receive timer handler=======
void rx_handler(unsigned long data)
{
	S_DEV *dev = (S_DEV *) data;
	int i=0;
	char temp_buf[LOCAL_BUFF_SIZE];
	int flag=0;
	int ret;
	printk("we are in the rx timer handler \n");
	while(inb(dev->base_addr + 0x05) & 0x01)  //Receiver Data Ready Indicator
	{
		if(i < LOCAL_BUFF_SIZE)
		{
			temp_buf[i]=inb(dev->base_addr + 0x00);
			++dev->num_rx;
		}
		else
		{
			++dev->num_rx;
			++dev->num_drop;
		}
		flag=1;
		i++;
	}
	printk("Rx Handler : %d bytes data is read from H/W\n",dev->num_rx);
	ret = kfifo_put(dev->read_kfifo,temp_buf,i);		
	printk("Rx Handler : %d bytes data is written in to read kfifo\n",ret);
	printk("Rx Handler : %d bytes data is droped\n",dev->num_drop);
	
	//if(timer_pending(&(dev->rx_timer))==0)
	{
		mod_timer(&(dev->rx_timer),jiffies + dev->poll_int_rx);  
		printk("we have activated rx timer from rx handler method\n"); 
	}
	if(flag==1)
		wake_up_interruptible(&(dev->read_queue));
}



S_DEV *my_dev;
int base_addr,kfifo_size,poll_int_tx,poll_int_rx;
module_param(base_addr,int,S_IRUGO);
module_param(poll_int_rx,int,S_IRUGO);  //you may consider 2 different 
module_param(poll_int_tx,int,S_IRUGO);  //you may consider 2 different 
                                     //parameters for the tx and rx s/w timers
module_param(kfifo_size,int,S_IRUGO);
struct resources *rs;
static int __init serial_init(void)
{
	
	if(alloc_chrdev_region(&serial_dev,0,1,"serial_driver"))
	{
		printk("error in device id creation\n");
		return -EBUSY;
	}
	printk("1. Allocate device ids\n ");

	//creating my_dev obj
	my_dev=kmalloc(sizeof(S_DEV),GFP_KERNEL);
	if(my_dev==NULL)
	{
		printk("error in my_dev allocation\n");
		unregister_chrdev_region(serial_dev,1);
		return -ENOMEM;
	}
	printk("2. Allocate device object\n ");
	my_dev->base_addr = base_addr;
	my_dev->kfifo_size = kfifo_size;
	my_dev->poll_int_rx = poll_int_rx;
	my_dev->poll_int_tx = poll_int_tx;
	my_dev->num_tx = 0;
	my_dev->num_rx = 0;

	//allocating base addrs to my_dev
	rs = (struct resources *)request_region(my_dev->base_addr,SERIAL_NO_PORTS,"serial_driver");
	if(rs==NULL)
	{
		printk("error in base addr allocation\n");
		unregister_chrdev_region(serial_dev,1);
		kfree(my_dev);
		return -EBUSY;
	}
	printk("3. Allocate base address\n ");

        //as per rules of s/w timers, we must initialize 
        //s/w timer objects - typically, we must setup corresponding
        //s/w timer handler and ptr to a private object !!!
        //
        //once s/w timer objects are initialized, they must 
        //be filled with appropriate future time-stamp(expiry time)
        //and activated - meaning, added to the s/w timer subsystem 
        //
        //once added, they will be processed for their time-stamp
        //once - no more - if we wish to re-activate a s/w timer,
        //we must repeat the process of setting up future time-stamp
        //and activating / triggering - this process must be repeated
        //as many times as needed !!!
	init_timer(&(my_dev->tx_timer));
	init_timer(&(my_dev->rx_timer));
	printk("3a. timer initialized\n ");
	my_dev->tx_timer.function = tx_handler;
	my_dev->tx_timer.data = (unsigned long)my_dev;
	my_dev->rx_timer.function = rx_handler;
	my_dev->rx_timer.data = (unsigned long)my_dev;
	printk("3b. timer handler assigned\n ");

	//allocating read buffer
	my_dev->read_buff = kmalloc(my_dev->kfifo_size,GFP_KERNEL);
	if(my_dev->read_buff==NULL)
	{
		printk("error in read buff allocation\n");
		unregister_chrdev_region(serial_dev,1);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_PORTS);
		return -ENOMEM;
	}
	printk("4. Allocate read buff\n ");
	//allocating write buff
	my_dev->write_buff = kmalloc(my_dev->kfifo_size,GFP_KERNEL);
	if(my_dev->write_buff==NULL)
	{
		printk("error in write buff allocation\n");
		unregister_chrdev_region(serial_dev,1);
		kfree(my_dev->read_buff);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_PORTS);
		return -ENOMEM;
	}
	printk("5. Allocate write` buff\n ");

	//initialization of wait queues
	init_waitqueue_head(&(my_dev->read_queue));
	init_waitqueue_head(&(my_dev->write_queue));
	printk("6. wait queues initialized\n ");

	//spinlocks for read & write initialization
	spin_lock_init(&(my_dev->spinlock_rd));
	spin_lock_init(&(my_dev->spinlock_wr));
	spin_lock_init(&(my_dev->lock));
	printk("7. spinlock initiliaezed\n ");
	
	//creating kfifos for read & write
	my_dev->read_kfifo = kfifo_init(my_dev->read_buff,my_dev->kfifo_size,GFP_KERNEL,&(my_dev->spinlock_rd));
	if(my_dev->read_kfifo==NULL)
	{
		printk("error in read kfifo buff allocation\n");
		unregister_chrdev_region(serial_dev,1);
		kfree(my_dev->read_buff);
		kfree(my_dev->write_buff);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_PORTS);
		return -ENOMEM;
	}
	printk("8. Allocate read kfifo buff\n ");
	
	my_dev->write_kfifo = kfifo_init(my_dev->write_buff,my_dev->kfifo_size,GFP_KERNEL,&(my_dev->spinlock_wr));
	if(my_dev->write_kfifo==NULL)
	{
		printk("error in read kfifo buff allocation\n");
		unregister_chrdev_region(serial_dev,1);
		kfifo_free(my_dev->read_kfifo);
		kfree(my_dev->write_buff);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_PORTS);
		return -ENOMEM;
	}
	printk("9. Allocate write kfifo buff\n ");

	//cdev initalization and addition
	cdev_init(&my_dev->cdev,&serial_fops);
	printk("10. cdev initialized\n ");

	kobject_set_name(&(my_dev->cdev.kobj),"device");
	
	my_dev->cdev.ops = &serial_fops;

	if(cdev_add(&my_dev->cdev,serial_dev,1))
	{
		printk("error in cdev\n");
		kobject_put(&(my_dev->cdev.kobj));
		unregister_chrdev_region(serial_dev,1);
		kfifo_free(my_dev->read_kfifo);
		kfifo_free(my_dev->write_kfifo);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_PORTS);
		return -EBUSY;
	}
	printk("11. cdev added\n ");
	
	serial_class = class_create(THIS_MODULE,"serial_driver");
	if(serial_class==NULL)
	{
		printk("Error in class creation\n");
		cdev_del(&(my_dev->cdev));
		unregister_chrdev_region(serial_dev,1);
		kfifo_free(my_dev->read_kfifo);
		kfifo_free(my_dev->write_kfifo);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_PORTS);
		return -ENOMEM;
	}
	
	device = class_device_create(serial_class,NULL,serial_dev,NULL,"serial_port");
	if(device == NULL)
	{
		printk("Error in class creation\n");
		class_destroy(serial_class);
		cdev_del(&(my_dev->cdev));
		unregister_chrdev_region(serial_dev,1);
		kfifo_free(my_dev->read_kfifo);
		kfifo_free(my_dev->write_kfifo);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_PORTS);
		return -ENOMEM;
	}
	
	device->class_data =  my_dev;
	
	class_device_create_file(device,&class_device_attr_tx);
	class_device_create_file(device,&class_device_attr_rx);
	class_device_create_file(device,&class_device_attr_base_addr);
	class_device_create_file(device,&class_device_attr_transmitter_poll_int);
	class_device_create_file(device,&class_device_attr_receiver_poll_int);


	printk("Serial : loaded\n");
	return 0;
}

static void __exit serial_exit(void)
{
	class_device_destroy(serial_class,serial_dev);
	class_destroy(serial_class);
	cdev_del(&(my_dev->cdev));
	unregister_chrdev_region(serial_dev,1);
	kfifo_free(my_dev->read_kfifo);
	kfifo_free(my_dev->write_kfifo);
	kfree(my_dev);
	release_region(my_dev->base_addr,SERIAL_NO_PORTS);
	printk("Serial : unloaded\n");
}

module_init(serial_init);
module_exit(serial_exit);


MODULE_DESCRIPTION("a minimal kernel polling driver with sw timers");

MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");

