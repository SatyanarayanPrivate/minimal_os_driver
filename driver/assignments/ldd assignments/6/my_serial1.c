#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/version.h>
#include<linux/device.h>
#include<linux/ioport.h>
#include<linux/pci.h>
#include<linux/slab.h>
#include<linux/vmalloc.h>
#include<linux/fs.h>
#include<linux/interrupt.h>
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
#include<linux/major.h>
#include<asm/unistd.h>
#include<asm/uaccess.h>
#include<asm/io.h>
#include<asm/fcntl.h>
#include<asm/errno.h>
#include<asm/ioctl.h>

#define MAX_BUFFSIZE (2 * 1024)
#define SERIAL_NO_REGISTERS 8  //this data is from data-sheet
#define HW_FIFO_SIZE 16   		//from data sheet
int ndevice = 1; 			//no of the devices
//int base_addr = 0x3F8,irq_no = 4;

//static char *plp_kmem_buffer;

static struct class *pseudo_class;	// pretend /sys/class 
static dev_t my_dev;			// dynamically assigned char device
//static struct cdev *plp_kmem_cdev;	// dynamically allocated at runtime

static int pcdd_open(struct inode *inode, struct file *file);
static int pcdd_release(struct inode *inode, struct file *file);
static ssize_t pcdd_read(struct file *file, char __user *buf,size_t count, loff_t *ppos);
static ssize_t pcdd_write(struct file *file, const char __user *buf,size_t count, loff_t *ppos);

// file operation of private object
typedef struct my_serial_dev{
	struct list_head list;
	struct cdev cdev;
	int base_addr;
	int irq_no;
	void *write_buff,*read_buff;	
	int kfifo_size;
	struct kfifo write_kfifo;
	struct kfifo read_kfifo;
	spinlock_t spinlock1;
        spinlock_t spinlock2;
        wait_queue_head_t queue1, queue2;	//queue1 for read and queue2 for write
        //struct tasklet_struct tx;
        //struct tasklet_struct rx; 

}c_dev;

//---------------------------------------interrupt handler--------------------------------------

irqreturn_t custom_serial_intr_handler(int irq_no, void *dev)
{
	int ret_val,i,j;
	char lk_buff[16];
	irqreturn_t irq_r_flag=0;
	char bytes;
	c_dev *dev1=dev;
	irq_r_flag |= IRQ_NONE;
	printk("interrupt handler\n");
	//check the status bits in LSR 	for TX HW FIFO empty 
	//if true, start transmitting certain no. of bytes to the tx HW FIFO	
	if( inb(dev1->base_addr+0x05) & 0x20)			//check for tx h/w fifo empty & tx fifo 
  	{						//not empty, then we copy the data into tx 
							//h/w fifo
		printk("in the isr\n");		
		bytes = kfifo_out(&(dev1->write_kfifo),lk_buff,HW_FIFO_SIZE);
		if(bytes!=0)
		{
			for(i=0;i<bytes; i++)
                	outb(lk_buff[i],dev1->base_addr+0x0);
		}
		bytes = kfifo_len(&(dev1->write_kfifo));	//we check if tx kfifo len ==0, then we
		if(bytes==0)					//disable tx interrupt
			outb(0x01, dev1->base_addr+1);
		irq_r_flag |= IRQ_HANDLED;
        }
	printk("out of the loop1\n");
	
	if(inb(dev1->base_addr+0x05)&0x01)
	{
		for(j=0;;j++)	
		{
			if(inb(dev1->base_addr+0x05)&0x01)
			{
				lk_buff[j] = inb(dev1->base_addr+0x0);	
			}
			else 
				break;
		}
		//here we dumping the data read into intermediate rx sw buffer
			kfifo_in(&(dev1->read_kfifo),lk_buff,j);
			wake_up_interruptible (&(dev1->queue1));		//and we wake up the blocked process
			wake_up_interruptible (&(dev1->queue2));
			irq_r_flag |= IRQ_HANDLED; 
		
	}
	return irq_r_flag; 
}

//-----------------------------------------------------------------------------------------------

static int pcdd_open(struct inode *inode, struct file *file)
{
	c_dev *dev;
	int ret;
	//int irq_no;
  	dev= container_of(inode->i_cdev,c_dev,cdev);
	file->private_data = dev;
	outb(0x83, dev->base_addr+3);		//LCR
	outb(0x01, dev->base_addr+1);		//IER
	outb(0x00, dev->base_addr+1);		//DLM
	outb(0x0c, dev->base_addr+0);		//DLL
	outb(0xc7, dev->base_addr+2);		//FCR
	dev->irq_no = 4;
	dev->base_addr = 0x3F8;
	ret=request_irq(dev->irq_no,custom_serial_intr_handler, IRQF_SHARED,
                               "custom_serial_device0",dev);
	if(ret != 0)
	{
			printk(KERN_ALERT "error in request_irq\n");
			
                        //must free resources allocated in open
                        //before this operation
                        return -EBUSY;
	}
	outb (0x08, dev->base_addr + 4);		//MCR
	return 0;
}
//--------------------------------------------------------------------------------------------------

static int pcdd_release(struct inode *inode, struct file *file)
{
	c_dev *dev;
	//int ret;
	dev = file->private_data; 
	//completed after some time

}
//-------------------------------------------------------------------------------------------------

static ssize_t pcdd_read(struct file *file,char __user *buf,size_t count, loff_t *ppos)
{
	c_dev *dev= file->private_data;
	int bytes, ret;
	unsigned char buffer[MAX_BUFFSIZE];
	printk ("read call\n");
	bytes = kfifo_len(&(dev->read_kfifo));
	if(bytes == 0)
	{
			printk("flag\n");
			if (file->f_flags & O_NONBLOCK)
			{			
				printk("file->f_flag\n");			
				return -EAGAIN;
			}
			else
			{
				printk("wait even interrupt\n");
				wait_event_interruptible(dev->queue1,kfifo_len(&(dev -> read_kfifo))!= 0);
				printk("out of wait event interruptible\n");
			}
	}
	if(access_ok (VERIFY_WRITE, (void __user *) buf, (unsigned long) count))
	{
		printk("access_ok\n");		
		bytes = kfifo_out_locked(&(dev->read_kfifo),buffer,bytes,&(dev->spinlock1));
		printk("the read data = %s\n", buffer);
		
	}
	else
		return -EFAULT;
	ret = copy_to_user((void __user*)buf,buffer,bytes);
	printk("the read data = %s\n",buffer );
	printk("copy_to_user\n");
	return bytes;
}

//--------------------------------------------------------------------------------------------------

static ssize_t pcdd_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	c_dev *dev;
	dev = file->private_data;
	char lk_buff[16]; 	
	int bytes,bytes1,i;
	printk("write call\n");
	bytes = kfifo_avail(&(dev->write_kfifo));
	if(bytes == 0)			//if there is no space in the fifo then go to the waitqueue
	{
		if(file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(dev->queue2,kfifo_avail(&(dev->write_kfifo)) != 0);
	}
	if(access_ok (VERIFY_READ, (void __user *)buf, (unsigned long) count)) 
	{
		//check the fifo, if there is data in fifo then enable the tx interrupt 
		// else we write in the fifo		
		//bytes1 = kfifo_len(&(dev->write_kfifo));	
		bytes1 = kfifo_in_locked(&(dev->write_kfifo), (unsigned char *)buf, count, &(dev->spinlock2));

		printk("the written data = %s\n", (unsigned char *)buf);		
		//if bytes>16
				
		//copy 16 bytes from kfifo to lkbuf and then send to device(outb)
		//if( inb(dev->base_addr+0x05) & 0x20)
		//{	
			bytes = kfifo_out(&(dev->write_kfifo),lk_buff,HW_FIFO_SIZE);
			for(i=0;i<bytes; i++)
				outb(lk_buff[i],dev->base_addr+0x0);
		//}
	
		bytes = kfifo_len(&(dev->write_kfifo));		
		if(bytes>0)
		{
			printk("enable the tx and rx interrupt\n");			
			outb(0x03, dev->base_addr+1);    //enable tx and rx interrupt
		}
	}
	else
		return -EFAULT;
	
	
	
	return bytes1;	//must return the no of bytes written to the device
}
//-------------------------------------------------------------------------------------------------
static struct file_operations pcdd_fops = {
	.read		= pcdd_read,
	.write		= pcdd_write,
	.open		= pcdd_open,
	.release	= pcdd_release,
	.owner		= THIS_MODULE,//this is a must for the system
                                      //ptr to this module's object/structure
};

LIST_HEAD(dev_list);			//pointer to the LIST_HEAD


//--------------------------------------------------------------------------------------------------

c_dev *pri_dev;	
//module_param(base_addr,int,S_IRUGO);
module_param(kfifo_size,int,S_IRUGO);
//module_param(irq_no,int,S_IRUGO);
struct resources *rs;
static int __init my_serial_init(void)
{
	int  ret,i;
	
	i = alloc_chrdev_region(&my_dev, 0,ndevice, "custom_serial_driver");
	
	if(i==0)
	{
		printk("device get its major no.\n");
	}
	for(i=0;i<ndevice;i++)
	{	
		printk("major = %d\n",MAJOR(my_dev + i));
		printk("minor = %d\n",MINOR(my_dev + i));
	}
	printk("1 : get major  no\n");	
	
	
	//for n devices use for loop
	pri_dev= kmalloc(sizeof(c_dev),GFP_KERNEL);	//allocate memory to private obj(my_serial_dev)
	if(pri_dev == NULL)
	{
		printk("error in allocating memory to private object\n");
	}
	printk("2 : kmalloc for my_dev\n");
	pri_dev->base_addr = 0x3F8;
	rs = request_region(0x3F8,SERIAL_NO_REGISTERS,"custom_serial_device0");	 
        /*if(rs==NULL){ 
		printk("hello\n");		
		kfree(pri_dev);
		return -EBUSY;
	}*/

	
	list_add_tail(&pri_dev->list,&dev_list);	//add to list queue of device
	printk("3 : list add tail\n");

	pri_dev->read_buff = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);	
	if(pri_dev->read_buff == NULL)
	{
		printk("Error in allocating memory for device read buffer....\n");
	}
	
	pri_dev->write_buff = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);	
	if(pri_dev->write_buff == NULL)
	{
		printk("Error in allocating memory for device write buffer....\n");
	}
	printk("4 : kmalloc buffer\n");
	
	spin_lock_init(&(pri_dev->spinlock1));
	spin_lock_init(&(pri_dev->spinlock2));	
	printk("5:  spin_lock in  it\n");
		
	kfifo_init(&(pri_dev->write_kfifo),pri_dev->write_buff,MAX_BUFFSIZE );
	kfifo_init(&(pri_dev->read_kfifo),pri_dev->read_buff,MAX_BUFFSIZE );
	printk("6:  kfifo init\n");
		
	init_waitqueue_head(&(pri_dev->queue1));	//initilization of queue for read
	init_waitqueue_head(&(pri_dev->queue2));	//initilization of queue for write
	printk("7: queue init\n");

	//tasklet_init(&(pri_dev->tx), tasklet_tx, &pri_dev); 
    	//tasklet_init(&(pri_dev->rx), tasklet_rx, &pri_dev);
	printk("7: tasklets init\n"); 

	cdev_init(&pri_dev->cdev,&pcdd_fops);  
	kobject_set_name(&(pri_dev->cdev.kobj),"custom_serial_dev0");
	pri_dev->cdev.ops = &pcdd_fops;
	if(cdev_add(&pri_dev->cdev, my_dev, 1)) {
		kobject_put(&(pri_dev->cdev.kobj));
		unregister_chrdev_region(my_dev, 1);
		kfree(pri_dev);
		//goto error;
	}
	pseudo_class = class_create(THIS_MODULE, "custom_serial_class");
	device_create(pseudo_class, NULL, my_dev, NULL, "custom_serial_dev0");
	printk(KERN_INFO "pcdd: loaded.\n");
	return 0;
}



static void __exit my_serial_exit(void)
{
	device_destroy(pseudo_class, my_dev);
	class_destroy(pseudo_class);	
	cdev_del(&(pri_dev->cdev));//remove the registration of the driver/device
	kfifo_free(&(pri_dev->write_kfifo));
	kfifo_free(&(pri_dev->read_kfifo));		
	kfree(pri_dev);
	unregister_chrdev_region(my_dev,1); 
	release_region(0x3F8,SERIAL_NO_REGISTERS);	
	printk("i am exit from the pc\n");
}

module_init(my_serial_init);
module_exit(my_serial_exit);

/* define module meta data */

MODULE_DESCRIPTION("Demonstrate kernel memory allocation");

MODULE_ALIAS("memory_allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");	

