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
///////////////////////////////////////////////////

#define SERIAL_NO_PORTS 8
#define HW_BUFF_SIZE 16
#define LOCAL_BUFF_SIZE 128



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
	struct timer_list tx_timer;
	struct timer_list rx_timer;

}S_DEV;

//----------------x-----------------------

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
	//reset DLAB bit & setting stop bit = 1 & with no priordev_t serial_dev;
ity in LCR reg 
	outb(0x03,dev->base_addr + 0x03);
	//disabling the interrupt using IER reg
	outb(0x00,dev->base_addr + 0x01);
	//enabling the fifos for self clearing in FCR reg
	outb(0x07,dev->base_addr + 0x02);
	//enabling normal mode of UART in MCR
	outb(0x00,dev->base_addr + 0x04);
	
	mod_timer(&(dev->rx_timer),jiffies + dev->poll_int_rx);
	
	printk("driver : Device opened\n");
	return 0;
}



//--------------------x--------------------



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



//-----------------------------x-------------------------------


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
	kfifo_out(&(dev1->read_kfifo),temp_buff,j);	
	printk("Rx Handler : %d bytes data is written in to read kfifo\n",ret);
	printk("Rx Handler : %d bytes data is droped\n",dev->num_drop);
	
	if(timer_pending(&(dev->rx_timer))==0)
	{
		mod_timer(&(dev->rx_timer),jiffies + dev->poll_int_rx);  
		printk("we have activated rx timer from rx handler method\n"); 
	}
	if(flag==1)
		wake_up_interruptible(&(dev->read_queue));
}


//-----------------x---------------------------

static ssize_t serial_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)dev_t serial_dev;

{
unsigned int  ret_val;
//	unsigned char ch;
          P_SERIAL_DEV *dev = file->private_data ;  

	mod_timer(&(dev->tx_timer),jiffies + dev->poll_int_tx);

        printk ("in dual_uart_write()...\n");
	
	ret_val = kfifo_avail(&(dev->write_kfifo));

	if(ret_val == 0)
	{
	if (file->f_flags & O_NONBLOCK)
		return -EAGAIN;	
	else
		wait_event_interruptible (dev->write_queue, kfifo_avail(&(dev->write_kfifo))!=0);
	}
	
	if (access_ok (VERIFY_READ, (void __user *) buf, (unsigned long) count))
	{
		ret_val = kfifo_in(&(dev->write_kfifo), buf, count);
	}
	else
	{
		return -EFAULT;
	}


}


void tx_handler(unsigned long data)
{
	int ret_val,i,j;
	irqreturn_t irq_r_flag = 0;
	P_SERIAL_DEV *dev1 = dev;
//	char byte;
	char lk_buff[MY_BUFSIZE];

	


	if(inb(dev1->base_addr +0x05) & 0x20 )
	{
		if(ret_val!= 0)
		{
			for(i=0;i<ret_val; i++)
				outb(lk_buff[i],dev1->base_addr+0x0);
		}
		
	}

	}
	if(timer_pending(&(dev->tx_timer))==0)
	{
		mod_timer(&(dev->tx_timer),jiffies + dev->poll_int_tx);  read
		printk("we have activated tx timer from tx handler method\n"); 
	}
	
		wake_up_interruptible(&(dev->write_queue));

}




//-----------------x------------------------
static struct file_operations serial_fops = {
	.open    = serial_open,
	.release = serial_release,
	.read    = serial_read,
	.write   = serial_write,
	.owner   = THIS_MODULE,
};



//---------------------x--------------------

S_DEV *my_dev;
int base_addr,kfifo_size,poll_int_tx,poll_int_rx;
module_param(base_addr,int,S_IRUGO);
module_param(poll_int_rx,int,S_IRUGO);  //you may consider 2 different 
module_param(poll_int_tx,int,S_IRUGO);  //you may consider 2 different 
                                     //parameters for the                                 tx and rx s/w timers

module_param(kfifo_size,int,S_IRUGO);

struct resources *rs;

static int __init serial_init(void)
{my_dev->base_addr = base_addr;
	my_dev->kfifo_size = kfifo_size;
	my_dev->poll_int_rx = poll_int_rx;
	my_dev->poll_int_tx = poll_int_tx;
	my_dev->num_tx = 0;
	my_dev->num_rx = 0;
	
	if(alloc_chrdev_region(&serial_dev,0,1,"serial_driver"))
	{
		printk("error in device id creation\n");dev_t serial_dev;

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

