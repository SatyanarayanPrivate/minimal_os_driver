/*
 * plp_kmem.c - Minimal example kernel module.
 */
#include<linux/module.h>
#include<linux/kfifo.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include<linux/pci.h>
#include<linux/sched.h>
#include<linux/types.h>
#include<linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/fs.h>
#include<linux/ioport.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/cdev.h>
#include<linux/moduleparam.h>
#include <asm/uaccess.h>
#include<asm/fcntl.h>
#include<asm/irq.h>
#include<asm/ioctl.h>
#include<asm/errno.h>
#include<linux/version.h>



#define CUST_SERIAL_NO_REGISTERS 8   
#define HW_FIFO_SIZE 16  
#define PLP_KMEM_BUFSIZE (1024*1024) 

/* global variables */

int ndevices = 1;

//static char *plp_kmem_buffer;

//static struct class *pseudo_class;	/* pretend /sys/class */
static dev_t plp_kmem_dev;		/* dynamically assigned char device */
static struct cdev *plp_kmem_cdev;	/* dynamically allocated at runtime. */

/* function prototypes */

//static int __init plp_kmem_init(void);
//static void __exit plp_kmem_exit(void);

static int plp_kmem_open(struct inode *inode, struct file *file);
static int plp_kmem_release(struct inode *inode, struct file *file);
static ssize_t plp_kmem_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos);
static ssize_t plp_kmem_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos);


/* file_operations */
typedef struct p_serial_dev{
	struct list_head list;
	struct cdev cdev;
	int base_addr;
	int irq_no;
	int kfifo_size;
	struct kfifo *write_kfifo;
	struct kfifo *read_kfifo;
        unsigned char *read_buff;
        unsigned char *write_buff;
        wait_queue_head_t read_queue;
        wait_queue_head_t write_queue;
 
}P_SERIAL_DEV;

static struct file_operations plp_kmem_fops = {
	.read		= plp_kmem_read,
	.write		= plp_kmem_write,
	.open		= plp_kmem_open,
	.release	= plp_kmem_release,
	.owner		= THIS_MODULE,//this is a must for the system
                                      //ptr to this module's object/structure
};

LIST_HEAD(dev_list);


static int plp_kmem_open(struct inode *inode, struct file *file)
{

  //struct pseudo_dev_obj *obj;
	P_SERIAL_DEV *dev;int ret;
  	dev = container_of(inode->i_cdev,P_SERIAL_DEV, cdev);
	file->private_data = dev;
		printk(KERN_ALERT " IN OPEN MODE\n");
  dump_stack(); 

	//outb (0x80, dev->base_addr + 3);    // LCR : bit-7: 1 for DLAB select
       //following initialization of the hw controller is 
       //based on data-sheet and certain recommendations
       //refer to data-sheet and other pdfs provided 

	outb(0x80, dev->base_addr+3);    // LCR : bit-7: 1 for DLAB select
        outb(0x00, dev-> base_addr+1);    // DLM : baud rate higher byte
        outb(0x0c, dev->base_addr+0);    // DLL : baud rate lower byte
        outb(0x03, dev->base_addr+3);    // LCR : bit data; 1 stop bits ; no parity ;
        outb(0x03, dev->base_addr+1); // IER : Rx and Tx interrupt enable
        outb(0xc7, dev->base_addr+2);

	ret=request_irq(irq_no,custom_serial_intr_handler,IRQF_DISABLED|IRQF_SHARED,
                               "custom_serial_device0",dev);
	
	if(ret != 0)
	{
			printk(KERN_ALERT "Unable to Register Interrupt for Device\n");
			
             		outb(0x00, dev->base_add+4); //MCR disable
			outb(0x00, dev->base_add+1); // IER disable
            
                        return -EBUSY;
	}

        //this is not documented in the data-sheet - refer to 
        //pdfs provided - this is specific to a particular 
        //hw implementation !!!
        outb (0x08, base_addr + 4);    // MCR : interrupt enable 

#ifdef PLP_DEBUG
	printk(KERN_DEBUG "plp_kmem: opened device.\n");
#endif

	return 0;  //success
}

/*
 * plp_kmem_release: Close the kmem device.
 */

static int plp_kmem_release(struct inode *inode, struct file *file)
{
	
       P_SERIAL_DEV *dev;
       dev = file->private_data; 

//dummy here - not always 

//typically, the following must be done
//stop h/w interrupts
//unload ISR from OS
//shutdown your bus-master DMA engine, if any 
//any other actions recommended by h/w 

#ifdef PLP_DEBUG
	printk(KERN_DEBUG "plp_kmem: device closed.\n");
#endif
	

        outb (0x00, base_addr + 4);    // MCR : interrupt disable
        outb (0x00, base_addr + 1);    // IER : Rx and Tx interrupt disable

        //wait until all interrupts are handled - you do it ??
	
	
	
	 while(in_interrupt()!=0)
	{
		continue;
	}
        free_irq(dev->irq_no, dev); //must free the irq in release along
        //free_irq(irq_no, file->private_data); //must free the irq in release along
                                              //with the private object ptr 
         
	return 0;
}




/*
 * plp_kmem_read: Read from the device.
 */

static ssize_t plp_kmem_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	P_SERIAL_DEV *dev= file->private_data;
	int bytes;
  printk ("read call\n");
  bytes = kfifo_len(&dev->read_kfifo); 

  if (bytes == 0)          
  {
    if (file->f_flags & O_NONBLOCK)
      return -EAGAIN;
    else
	wait_event_interruptible (dev->read_queue, kfifo_len(&dev->read_kfifo) != 0) ;
    }

  if (access_ok (VERIFY_WRITE, (void __user *) buf, (unsigned long) count))    
  {
    bytes = kfifo_out (&dev->read_kfifo,(void *) buf, count);
    return bytes;
  }
	else
return -EFAULT;
}

/*
 * plp_kmem_write: Write to the device.
 */
irqreturn_t custom_serial_intr_handler(int irq_no, void *dev)
{
static unsigned int drop=0,rcv=0;	
int ret_val,i,j;
char lk_buff[16];
irqreturn_t irq_r_flag=0;
char byte;

 irq_r_flag |= IRQ_NONE;


if(inb(base_addr+0x05 )& 0x20)
{       

        
	ret_val = kfifo_out(&dev->write_kfifo,lk_buff,HW_FIFO_SIZE);
	if(ret_val!=0)
	{
		for(i=0;i<ret_val; i++)
                outb(lk_buff[i],base_addr+0x0);

         }
	 wake_up_interruptible(&(dev->write_queue));

         
         irq_r_flag |= IRQ_HANDLED;

	
}


if(inb(base_addr+0x05)&0x01)
{
  for(j=0;j<HW_FIFO_SIZE;j++)  
  {
    if(inb(base_addr+0x05)&0x01)
    {

	lk_buff[j] = inb(base_addr+0x0);
    }

    else break;
  }
	if(kfifo_is_full(&dev->read_kfifo))
	{
		drop++;
		
	}
  	else{
		rcv++;
	
 		 kfifo_in(&dev->read_kfifo,lk_buff,HW_FIFO_SIZE);
	}

  
  wake_up_interruptible (&(dev->read_queue));
	

  
  irq_r_flag |= IRQ_HANDLED; 
}
  return irq_r_flag; 
}

static ssize_t plp_kmem_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
        

        

        struct P_SERIAL_DEV *dev = file->private_data ;  

        
  	unsigned int i, ret_val, bytes;
  	unsigned char ch;
  	 
 
  ret_val = kfifo_avail(&dev->write_kfifo);  
  
  	if (ret_val == 0)
  	{

    		if (file->f_flags & O_NONBLOCK)
      		return -EAGAIN;
    		else
		{
     		wait_event_interruptible(dev->write_queue,(kfifo_avail(&dev->write_kfifo))==0 );

		}
	}
  

	if (access_ok (VERIFY_READ, (void __user *) buf, (unsigned long) count))    
  	{
    		bytes = kfifo_in(&dev->write_kfifo, (void *)buf, count);    
  		
	}
  	else
  	{
      		return -EFAULT;
 	}
	return bytes;
}

/*
 * plp_kmem_init: Load the kernel module into memory
 */

struct resource *rs1; 




P_SERIAL_DEV *my_dev;



static int  base_addr,irq_no,kfifo_size1;
module_param(base_addr,int,S_IRUGO);
module_param(irq_no,int,S_IRUGO);
module_param(kfifo_size1,int,S_IRUGO);

static int __init custom_serial_init(void)
{
	int ret;

	my_dev = kmalloc(sizeof(P_SERIAL_DEV),GFP_KERNEL);


	rs1 = request_region(base_addr,NO_REG_ADDRESSES,"custom_serial_device0");	
        if(rs1==NULL){ 
		kfree(my_dev);
		return -EBUSY;
	}
	
	list_add_tail(&my_dev->list,&dev_list);

	my_dev->write_buff = kmalloc(MAX_BUFFER_AREA,GFP_KERNEL);
	  if(my_dev->write_buff==NULL)
	  {
	      printk("error in write_buff's kmalloc()...\n");
              kfree(my_dev);
              release_region(base_addr,CUST_SERIAL_NO_PORTS);  
	      return -ENOMEM;
	  }
	  my_dev->read_buff = kmalloc(MAX_BUFFER_AREA,GFP_KERNEL);

        if(my_dev->read_buff==NULL)
          {
              printk("error in read_buff's kmalloc()...\n");
              kfree(my_dev->write_buff);
              kfree(my_dev); 
              release_region(base_addr,CUST_SERIAL_NO_PORTS);  
              return -ENOMEM;
          }
  // spin_lock_init(&(my_dev->spinlock_rd));
   //spin_lock_init(&(my_dev->spinlock_wr));
   	
      kfifo_init (&my_dev->write_kfifo,my_dev->write_buff, MAX_BUFFER_AREA);
    if(&my_dev->write_kfifo==NULL)
    {
        printk("error in write_kfifo_init...\n");
        return 0;
    }
   
      kfifo_init (&my_dev->read_kfifo,my_dev->read_buff, MAX_BUFFER_AREA);
    if(&my_dev->read_kfifo==NULL)
    {
        printk("error in read_kfifo_init...\n");
        return 0;
    }
 
    init_waitqueue_head(&my_dev->write_queue);
    init_waitqueue_head(&my_dev->read_queue); 

	if (alloc_chrdev_region(&plp_kmem_dev, 0,1, "custom_serial_driver"))
		goto error;


        cdev_init(&my_dev->cdev,&plp_kmem_fops);  


        kobject_set_name(&(my_dev->cdev.kobj),"custom_serial_dev0");
	my_dev->cdev.ops = &plp_kmem_fops;
	
        
        
        if (cdev_add(&my_dev->cdev, plp_kmem_dev, 1)) {
		kobject_put(&(my_dev->cdev.kobj));
		unregister_chrdev_region(plp_kmem_dev, 1);
		kfree(my_dev);
		goto error;
	}


	printk(KERN_INFO "plp_kmem: loaded.\n");

	return 0;

error:{
	printk(KERN_ERR "plp_kmem: cannot register device.\n");
	kfifo_free(&my_dev->write_kfifo);
	kfifo_free(&my_dev->read_kfifo);
	cdev_del(&my_dec->cdev);
	kfree(my_dev);
	
	release_region(base_addr,CUST_SERIAL_NO_REGISTERS);
        
	return -EINVAL;
	}
}// close of init method


static void __exit custom_serial_exit(void)
{


	cdev_del(&(my_dev->cdev));

        kfifo_free(&my_dev->write_kfifo);
	kfifo_free(&my_dev->read_kfifo);

	unregister_chrdev_region(plp_kmem_dev,1);

       

        kfree(my_dev); 
	release_region(base_addr,CUST_SERIAL_NO_REGISTERS);
	printk(KERN_INFO "plp_kmem: unloading.\n");
}// close of exit method



module_init(custom_serial_init);
module_exit(custom_serial_exit);

/* define module meta data */

MODULE_DESCRIPTION("Demonstrate kernel memory allocation");

MODULE_ALIAS("memory_allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");
