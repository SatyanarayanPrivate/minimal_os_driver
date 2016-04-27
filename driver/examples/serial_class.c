/*
 * plp_kmem.c - Minimal example kernel module.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>

#define CUST_SERIAL_NO_REGISTERS 8  //this data is from data-sheet 
#define HW_FIFO_SIZE 16   //from data sheet
#define PLP_KMEM_BUFSIZE (1024*1024) /* 1MB internal buffer */

/* global variables */

int ndevices = 1;

static char *plp_kmem_buffer;

static struct class *pseudo_class;	/* pretend /sys/class */
static dev_t plp_kmem_dev;		/* dynamically assigned char device */
static struct cdev *plp_kmem_cdev;	/* dynamically allocated at runtime. */

/* function prototypes */

static int __init plp_kmem_init(void);
static void __exit plp_kmem_exit(void);

static int plp_kmem_open(struct inode *inode, struct file *file);
static int plp_kmem_release(struct inode *inode, struct file *file);
static ssize_t plp_kmem_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos);
static ssize_t plp_kmem_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos);

//a typical private object of a physical device instance will
//maintain sw and hw information - particularly, hw resources'
//info is maintained - in this context, base address of the 
//I/O controller is maintained and irq no of irq line associated
//with the I/O controller is maintained !!!
//if your I/O controller has more attributes maintain them as well
//however, if your I/O controller is intelligent, system will maintain
//most of their attributes in specific system related objects !!!
//
//

/* file_operations */
typedef struct p_serial_dev{
	struct list_head liste
	struct cdev cdev;
	int base_addr;
	int irq_no;
	int kfifo_size;
	struct kfifo *write_kfifo;
	struct kfifo *read_kfifo;
	spinlock_t spinlock1;
        spinlock_t spinlock2;
        unsigned char *read_buff;
        unsigned char *write_buff;
        wait_queue_head_t read_queue;
        wait_queue_head_t write_queue;
        struct tasklet_struct tx;
        struct tasklet_struct rx; 

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

/*
 * plp_kmem_open: Open the kmem device
 */
//open may be typically used to initialize sw resources and
//hw registers/resources, if required - there is no hard and 
//fast set of rules about open() - code as per your requirements !!1

static int plp_kmem_open(struct inode *inode, struct file *file)
{

  //struct pseudo_dev_obj *obj;
	P_SERIAL_DEV *dev;int ret;
  	dev = container_of(inode->i_cdev,struct P_SERIAL_DEV, cdev);
	file->private_data = dev;
		printk(KERN_ALERT " IN OPEN MODE\n");
  dump_stack(); 

	//outb (0x80, dev->base_addr + 3);    // LCR : bit-7: 1 for DLAB select
       //following initialization of the hw controller is 
       //based on data-sheet and certain recommendations
       //refer to data-sheet and other pdfs provided 

	outb (0x80, dev->base_addr + 3);    // LCR : bit-7: 1 for DLAB select
        outb (0x00, base_addr + 1);    // DLM : baud rate higher byte
        outb (0x0c, base_addr + 0);    // DLL : baud rate lower byte
        outb (0x03, base_addr + 3);    // LCR : bit data; 1 stop bits ; no parity ;
        outb (0x03, base_addr + 1);    // IER : Rx and Tx interrupt enable
        outb (0xc7, base_addr + 2);    // FCR : interrupt for 14 byte, enable fifo


	//ret=request_irq(irq_no,custom_serial_intr_handler,
        //                IRQF_DISABLED|IRQF_SHARED,
        //                       "custom_serial_device0",dev);
	ret=request_irq(irq_no,custom_serial_intr_handler,
                        0,"custom_serial_device0",dev);
	if(ret != 0)
	{
			printk(KERN_ALERT "Unable to Register Interrupt for Device\n");
			
                        //must free resources allocated in open
                        //before this operation
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
  bytes = __kfifo_len(dev->read_kfifo); //returns number of bytes in kfifo...

  if (bytes == 0)           /*if no data in kfifo */
  {
    if (file->f_flags & O_NONBLOCK)
      return -EAGAIN;
    else
	wait_event_interruptible (dev->read_queue,\
                      __kfifo_len(dev->read_kfifo) != 0) );
    }

  if (access_ok (VERIFY_WRITE, (void __user *) buf, (unsigned long) count))    // 1:success; 0:error
  {
    bytes = __kfifo_get (dev->read_kfifo, buf, count);
    return bytes;
  }
	else
return -EFAULT;
}
e
void tasklet_rx(unsigned long private_obj)
{
    short int i=0,bytes;
    P_SERIAL_DEV *dev = (P_SERIAL_DEV *) private_obj;

    //you may also shift the copying of data from local buffer to kfifo here 
    //it will make the interrupt handler lighter 

    wake_up_interruptible (&(dev->read_queue));
    
} 

void tasklet_tx(unsigned long private_obj)
{
    short int i=0,bytes;
    P_SERIAL_DEV *dev = (P_SERIAL_DEV *) private_obj;

    //considering Tx is not a time critical activity
    //compared to received, most of the Tx activity
    //can be moved to tasklet_tx's method !!!!

    wake_up_interruptible (&(dev->write_queue));
    
}

/*
 * plp_kmem_write: Write to the device.
 */
irqreturn_t custom_serial_intr_handler(int irq_no, void *dev){
	
int ret_val,i,j;
irqreturn_t irq_r_flag=0;
char byte;

 irq_r_flag |= IRQ_NONE;

//check the status bits in LSR 	for TX HW FIFO empty 
//if true, start transmitting certain no. of bytes to the tx HW FIFO
if(inb(base_addr+0x05 )& 0x20)
{       

        //kfifo_get() is a non-blocking API that will 
        //read as much data as possible from the intermediate tx sw kfifo 
        //
	ret_val = kfifo_get(dev->write_kfifo,lk_buff,HW_FIFO_SIZE);
	if(ret_val!=0)
	{
		for(i=0;i<ret_val; i++)
                outb(lk_buff[i],base_addr+0x0);

         }

         //tasklet_schedule(&(dev->tx));
         irq_r_flag |= IRQ_HANDLED;

}
//here we are checking the rx HW fIFO status and reading
//as much data that may be read into a local buffer
if(inb(base_addr+0x05)&0x01)
{
  for(j=0;;j++)  //this code is not the best way to write ??
  {
    if(inb(base_addr+0x05)&0x01)
    {

	lk_buff[j] = inb(base_addr+0x0);
    }

    else break;
  }
  //here we dumping the data read into intermediate rx sw buffer
  kfifo_put(dev->read_kfifo,lk_buff,j);
  //tasklet_schedule(&(dev->rx));
  wake_up_interruptible (&(dev->read_queue));
  
  irq_r_flag |= IRQ_HANDLED; 
}

  return rq_r_flag; 
}

static ssize_t plp_kmem_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
        

        //fetching the current device object/context

        struct P_SERIAL_DEV *dev = file->private_data ;  

        //write to the kfifo, as per the standard rules
        //and return appropriately

        //the special case of blocking

	printk ("in dual_uart_write()...\n");

  unsigned int i, ret_val, bytes;
  unsigned char ch;
  char lk_buff[16]; 
 
  ret_val = __kfifo_len (dev->write_kfifo);    // checking kfifo for free space...
  ret_val = kfifo_size - ret_val; // available free space in kernel buffer...
  if (ret_val == 0)
  {

    if (file->f_flags & O_NONBLOCK)
      return -EAGAIN;
    else
      wait_event_interruptible (dev->write_queue, (kfifo_size - kfifo_len(dev->write_kfifo)>0 );

}
    //access_ok() is a must - it verifies whether the user-space buffer is really 
    //part of user-space or is it part of kernel-space 
    //this is to avoid some malicious users of our driver to corrupt 
    //kernel space objects - ideally, no good user will do this, but not all users are good 

    if (access_ok (VERIFY_READ, (void __user *) buf, (unsigned long) count))    // 1:success; 0:erro
  {
    ret_val = __kfifo_put (dev->write_kfifo, buf, count);    //copy from user to kfifo
  }
  else
  {
      return -EFAULT;
  }

  if( inb(base_addr+0x05) & 0x20)
  {

     bytes = kfifo_get(dev->write_kfifo,lk_buff,HW_FIFO_SIZE);

     for(i=0;i<bytes; i++)
               outb(lk_buff[i],base_addr+0x0);
            
   }
	return ret_val;  //must return the no of bytes written to the device
}

/*
 * plp_kmem_init: Load the kernel module into memory
 */

struct resource *rs1; 




P_SERIAL_DEV *my_dev;

//in this context, we will be passing parameters to provide 
//base addresses and irq nos - meaning, we will be using a 
//static/fixed  approach, which may not be true always - 
//there are scenarios, which may require dynamic addresses/
//irq nos - we may use different techniques to handle dynamic
//resource allocations, in other contexts !!!
//
//refer to chapter12 of LDD/3, which discussed one such 
//mechanism using PCI bus !!!
//
//refer to pdfs provided along with the hw related assignments-
//static resources and their details are provided !!


int base_addr,irq_no,kfifo_size;
module_param(base_addr,int,S_IRUGO);
module_param(irq_no,int,S_IRUGO);
module_param(kfifo_size,int,S_IRUGO);

static int __init custom_serial_init(void)
{
	int ret;

	my_dev = kmalloc(sizeof(P_SERIAL_DEV),GFP_KERNEL);

        //in this context, we are assuming that the base address
        //and allocated block of addresses for the I/O controller
        //are located in I/O space, not memory space !!! we need
        //a block of addresses starting from base address to 
        //address several registers of a device controller - 
        //refer to data sheet of your device controller !!!
        //
        //before using I/O space addresses, a driver must lock
        //the addresses using request_region() and verify that 
        //it is allowed to lock  the addresses - it may not be
        //allowed to lock the addresses, if another driver 
        //has been loaded and locked the addresses - if so,
        //our driver must return error and must not be loaded!!!

        //if base address and allocated block of addresses are 
        //located in memory space, our driver must lock such 
        //addresses using request_mem_region() !!!

	rs1 = request_region(base_addr,NO_REG_ADDRESSES,"custom_serial_device0");	
        if(rs1==NULL){ 
		kfree(my_dev);
		return -EBUSY;
	}
	
	list_add_tail(&my_dev->list,&dev_list);

        //we will be using kfifos in this context to 
        //serve as intermediate buffers between driver
        //and device controllers hw buffers - in this context, 
        //there will be a tx kfifo supporting tx hw buffers and 
        //one rx kfifo supporting rx hw buffers - more details
        //can be found in assignment pdf !!!
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
   spin_lock_init(&(my_dev->spinlock_rd));
   spin_lock_init(&(my_dev->spinlock_wr));
   my_ dev->write_kfifo =
      kfifo_init (dev->write_buff, MAX_BUFFER_AREA, GFP_KERNEL, &(my_dev->spinlock_wr));
    if(my_dev->write_kfifo==NULL)
    {
        printk("error in write_kfifo_init...\n");
        return 0;
    }
   my_dev->read_kfifo =
      kfifo_init (dev->read_buff, MAX_BUFFER_AREA, GFP_KERNEL, &(my_dev->spinlock_rd));
    if(my_dev->read_kfifo==NULL)
    {
        printk("error in read_kfifo_init...\n");
        return 0;
    }
    //here, we need 2 wait queues - one wq support tx 
    //part of the device/driver
    //another supports rx part of the device/driver
    //you will find more details in the assignment pdf 
 
    init_waitqueue_head(&dev->write_queue);
    init_waitqueue_head(&dev->read_queue); 

    //initially, do not use bottom half mechanisms - 
    //one such popular mechanism that is used here 
    //is tasklets !!!

    tasklet_init(&(my_dev->tx), tasklet_tx, &my_dev); 
    tasklet_init(&(my_dev->rx), tasklet_rx, &my_dev); 
    


        //the first param. is the storage for the first device no
        //allocated dynamically

        //second param. is the minor no. associated with the first
        //device no. allocated dynamically

        //third param. is the no. of dynamic device ids. requested
        //last param. is a logical name 
	if (alloc_chrdev_region(&plp_kmem_dev, 0,5, "custom_serial_driver"))
		goto error;

        //we are requesting for a system defined structure 
        //from a slab allocator in the KMA dedicated for
        //struct cdev 
        //you may ask the system to allocate or you may provide
        //the structure as a global data

	//if (0 == (plp_kmem_cdev = cdev_alloc()))
	//	goto error;

        cdev_init(&my_dev->cdev,&plp_kmem_fops);  

	//we are dealing with certain special objects of the I/O subsystem 

        kobject_set_name(&(my_dev->cdev.kobj),"custom_serial_dev0");
        //we are passing the file-operations supported by
        //our driver to the system - this is known as 
        //passing hooks - registering our driver's characteristics
        //with the system
	my_dev->cdev.ops = &plp_kmem_fops; /* file up fops */
	//ptr to cdev
        //first device id
        //no of devices to be managed by cdev
        if (cdev_add(&my_dev->cdev, plp_kmem_dev, 1)) {
		kobject_put(&(my_dev->cdev.kobj));
		unregister_chrdev_region(plp_kmem_dev, 1);
		kfree(my_dev);
		goto error;
	}

        //currently, do not include this section - we will do 
        //after understanding the LDD model and sysfs

	pseudo_class = class_create(THIS_MODULE, "custom_serial_class");
	if (IS_ERR(pseudo_class)) {
		printk(KERN_ERR "plp_kmem: Error creating class.\n");
		cdev_del(plp_kmem_cdev);
		unregister_chrdev_region(plp_kmem_dev, 1);
                //ADD MORE ERROR HANDLING
		goto error;
	}
	//device_create(pseudo_class, NULL, plp_kmem_dev, NULL, "pseudo_dev%d",i);
	device_create(pseudo_class, NULL, plp_kmem_dev, NULL, "custom_serial_dev0");

	printk(KERN_INFO "plp_kmem: loaded.\n");

	return 0;

error:
	printk(KERN_ERR "plp_kmem: cannot register device.\n");

        //return appropriate negative error code
	return -EINVAL;
}

/*
 * plp_kmem_exit: Unload the kernel module from memory
 */


static void __exit custom_serial_exit(void)
{

//many pending parts - SEE THE init for more ??

//      after LDD model and sysfs 
	device_destroy(pseudo_class, plp_kmem_dev);
	class_destroy(pseudo_class);

        //removing the registration of my driver
	cdev_del(&(my_dev->cdev));//remove the registration of the driver/device

        //freeing the logical resources - device nos.
	unregister_chrdev_region(plp_kmem_dev,1);

        //freeing the system-space buffer

        kfree(my_dev); 
	release_region(base_addr,CUST_SERIAL_NO_REGISTERS);
	printk(KERN_INFO "plp_kmem: unloading.\n");
}

/* declare init/exit functions here */

module_init(custom_serial_init);
module_exit(custom_serial_exit);

/* define module meta data */

MODULE_DESCRIPTION("Demonstrate kernel memory allocation");

MODULE_ALIAS("memory_allocation");
MODULE_LICENSE("GPL");
MODULE_VERSION("0:1.0");
