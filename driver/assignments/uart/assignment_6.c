/*#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/kfifo.h>
#include <linux/interrupt.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/fcntl.h>
#include <asm/irq.h>
#include <asm/errno.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/version.h>
*/
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
#include<linux/spinlock.h>


#define NO_OF_PORTS 8
#define MAX_BUFFER_AREA (1024*1024)
#define HW_FIFO_SIZE 16


int base_addr = 0x03f8,irq_no = 4;//kfifo_size = MAX_BUFFER_AREA;

module_param(base_addr,int,S_IRUGO);
module_param(irq_no,int,S_IRUGO);
module_param(kfifo_size,int,S_IRUGO);

struct resource *rs; 

typedef struct p_serial_dev
{
	struct list_head list;
	struct cdev cdev;
	//int base_addr;
	//int irq_no;
	//int kfifo_size;
	spinlock_t rd_spinlock;
	spinlock_t wr_spinlock;
	struct kfifo read_kfifo;
	struct kfifo write_kfifo;
	unsigned char *read_buff;
	unsigned char *write_buff;
	wait_queue_head_t read_queue;
	wait_queue_head_t write_queue;
               
}P_SERIAL_DEV;

/***************************** ==== Open Method ==== ********************************/

irqreturn_t pseudo_serial_intr_handler(int , void *);

static int plp_kmem_open(struct inode *inode, struct file *filp)
{


	P_SERIAL_DEV *dev;
	int ret;
	dev = container_of(inode->i_cdev,P_SERIAL_DEV, cdev);
	filp->private_data = dev;
		printk(" IN OPEN MODE\n");
	dump_stack(); 

	outb (0xc7, base_addr + 2); //FCR enable fifo + trigger level set
	outb (0x80, base_addr + 3); //LCR 7 set DLAB
	outb (0x00, base_addr + 1); //DLM : baud rate higher byte for baud rate 9600
	outb (0x0c, base_addr + 0); //DLL : baud rate lower byte for baud rate 9600
	outb (0x03, base_addr + 3); //set 8-bit data + set no parity + 1 stop bit + disable DLAB bit
	outb (0x01, base_addr + 1); //IER ,enabling receive buffer interrupt ERBFI
	outb (0x10, base_addr + 4); // loop-back
	
	ret=request_irq(irq_no,pseudo_serial_intr_handler,IRQF_DISABLED|IRQF_SHARED,"custom_serial_device0",dev);
	if(ret != 0)
	{
		printk(KERN_ALERT "Unable to Register Interrupt for Device\n");
                return -EBUSY;
        }
	outb (0x08, base_addr + 4);	// MCR : interrupt enable

	return 0;
}

/***************************** ==== Release Method ==== ********************************/

static int plp_kmem_release(struct inode *inode, struct file *filp)
{

       P_SERIAL_DEV *dev;
       dev = filp->private_data; 


	printk("plp_kmem: device closed.\n");

	outb (0x00, base_addr + 4);	// MCR : interrupt disable
	outb (0x00, base_addr + 1);	// IER : Rx and Tx interrupt disable

	while(in_interrupt());
        	free_irq(irq_no, dev);
      	return 0;
}

/***************************** ==== Read Method ==== ********************************/



static ssize_t plp_kmem_read(struct file *filp, char __user *buf,size_t count, loff_t *pos)
{
	P_SERIAL_DEV *dev= filp->private_data;
	int bytes;
	printk ("In read call\n");
	 
	if (kfifo_is_empty(&(dev->read_kfifo)))
	{
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(dev->read_queue,!kfifo_is_empty(&(dev->read_kfifo))) ;
	}

	if (access_ok (VERIFY_WRITE, (void __user *) buf, (unsigned long) count))
	{
		bytes = kfifo_out_locked(&(dev->read_kfifo),(void __user *) buf,count,&(dev->rd_spinlock));
		return bytes;
	}
	else
		return -EFAULT;
}

/***************************** ==== Write Method ==== ********************************/



static ssize_t plp_kmem_write(struct file *filp, const char __user *buf,size_t count, loff_t *pos)
{
	P_SERIAL_DEV *dev= filp->private_data;
	int bytes;
	printk ("In write call\n");
	 
	if (kfifo_is_full(&(dev->write_kfifo)))
	{
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(dev->write_queue,!kfifo_is_full(&(dev->write_kfifo))) ;
	}

	outb (0x03, base_addr + 1); //IER ,enabling Tx & Rx buffer interrupt ERBFI & EXBFI

	if (access_ok (VERIFY_READ, (void __user *) buf, (unsigned long) count))
	{
		bytes = kfifo_in_locked(&(dev->write_kfifo),(void __user *) buf,count,&(dev->wr_spinlock));
		return bytes;
	}
	else
		return -EFAULT;
}


/************* ====== initialising the file operations in the file operations object ===== ********/

static struct file_operations p_device_fops = {
	.owner		= THIS_MODULE,
	.read		= plp_kmem_read,
	.write		= plp_kmem_write,
	.open		= plp_kmem_open,
	.release	= plp_kmem_release
	
};

/***************************** ==== Interrupt Handler ==== ********************************/

unsigned long received=0,dropped=0;

irqreturn_t pseudo_serial_intr_handler(int irq_no, void *p_dev)
{
	int ret_val,i,j;
	irqreturn_t irq_r_flag=0;
	P_SERIAL_DEV *dev = (P_SERIAL_DEV *)p_dev;
	char lk_buff[HW_FIFO_SIZE];
	
	irq_r_flag |= IRQ_NONE;

	if(inb(base_addr+0x05) & 0x20) 		//comparing for Tx interrupt
	{
	
		outb (0x01, base_addr + 1); //IER ,disabling Tx & enabling Rx buffer interrupt ERBFI & EXBFI
		
		//copying the data into local buffer from the kfifo buffer

		ret_val = kfifo_out_locked(&(dev->write_kfifo),lk_buff,HW_FIFO_SIZE, &(dev->wr_spinlock));
		if(ret_val > 0)
		{
			for(i = 0; i < ret_val ; i++)
				outb(lk_buff[i], base_addr+0x00);
		}
		
		wake_up_interruptible (&(dev->write_queue));
	  
		irq_r_flag |= IRQ_HANDLED; 
	}	


	if(inb(base_addr+0x05) & 0x01) 		//comparing for Rx interrupt
	{
		for(j=0;j<16;j++)
		{
			if(inb(base_addr+0x05) & 0x01)
			{
				lk_buff[j] = inb(base_addr+0x00);
			}
			else
				break;
		}

		//copying the data from local buffer onto the kfifo buffer

		if(kfifo_is_full(&(dev->read_kfifo)))
			dropped += j;
		else
		{
			kfifo_in_locked(&(dev->read_kfifo),lk_buff,j,&(dev->rd_spinlock));
			received += j;
		}
	
		wake_up_interruptible(&(dev->read_queue));
	  
		irq_r_flag |= IRQ_HANDLED; 
	}

	

	return irq_r_flag; 
}


/***************************** ==== INIT MODULE ==== ********************************/


P_SERIAL_DEV *my_dev;
static dev_t p_device_id;

static int __init p_serial_init(void)
{

	my_dev = kmalloc(sizeof(P_SERIAL_DEV),GFP_KERNEL);
        if(my_dev==NULL)
        { 
		printk("Error in creating a private object...!\n");
		return -EBUSY;
	}


	rs = request_region(base_addr,NO_OF_PORTS,"pseudo_serial_device0");	
        if(rs==NULL)
        { 
		kfree(my_dev);
		return -EBUSY;
	}

	my_dev->read_buff = kmalloc(MAX_BUFFER_AREA,GFP_KERNEL);
        if(my_dev->read_buff==NULL)
        {
	      printk("error in read_buff's memory allocation...\n");
	      kfree(my_dev); 
	      release_region(base_addr,NO_OF_PORTS);  
	      return -ENOMEM;
	}
	
	kfifo_init(&(my_dev->read_kfifo),(void*)my_dev->read_buff, MAX_BUFFER_AREA);
	
	my_dev->write_buff = kmalloc(MAX_BUFFER_AREA,GFP_KERNEL);
        if(my_dev->write_buff==NULL)
	{
	      printk("error in read_buff's memory allocation...\n");
	      kfifo_free(&(my_dev->read_kfifo));
	      kfree(my_dev); 
	      release_region(base_addr,NO_OF_PORTS);  
	      return -ENOMEM;
	}
	
	kfifo_init(&(my_dev->write_kfifo),(void*)my_dev->read_buff, MAX_BUFFER_AREA);

	spin_lock_init(&(my_dev->wr_spinlock));
	spin_lock_init(&(my_dev->rd_spinlock));

	init_waitqueue_head(&(my_dev->read_queue));
	init_waitqueue_head(&(my_dev->write_queue));

	kobject_set_name(&(my_dev->cdev.kobj),"p_serial_dev0");
	
	if (alloc_chrdev_region(&p_device_id, 0,1, "pseudo_serial_driver"))
	{
		printk("Error in allocating the serial device driver region..!\n");
		kobject_put(&(my_dev->cdev.kobj));
              	kfifo_free(&(my_dev->read_kfifo));
              	kfifo_free(&(my_dev->write_kfifo));
		kfree(my_dev);
		release_region(base_addr,NO_OF_PORTS);
		return -EINVAL;
	}

	printk("Device ID is %u (MAJOR:%d,Minor:%d)\n",p_device_id,MAJOR(p_device_id),MINOR(p_device_id));	
	cdev_init(&my_dev->cdev,&p_device_fops);

      
	if ((cdev_add(&my_dev->cdev, p_device_id , 1)) < 0)
	{
		printk("Error in adding the serial device to the driver region..!\n");
		kobject_put(&(my_dev->cdev.kobj));
		unregister_chrdev_region(p_device_id, 1);
              	kfifo_free(&(my_dev->read_kfifo));
              	kfifo_free(&(my_dev->write_kfifo));
		kfree(my_dev);
		release_region(base_addr,NO_OF_PORTS);
		return -EINVAL;
	}
	return 0;

	
}

/***************************** ==== EXIT MODULE ==== ********************************/


static void __exit p_serial_exit(void)
{

	cdev_del(&(my_dev->cdev));

	unregister_chrdev_region(p_device_id,1);
	
	kfifo_free(&(my_dev->read_kfifo));
	kfifo_free(&(my_dev->write_kfifo));
        
	kfree(my_dev); 
	
	release_region(base_addr,NO_OF_PORTS);

	printk("plp_kmem: unloaded...!\n");
}

module_init(p_serial_init);
module_exit(p_serial_exit);

MODULE_LICENSE("GPL"); 

