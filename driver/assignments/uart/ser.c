#include <linux/kernel.h>
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

#include <linux/sched.h>




#define NO_OF_PORTS 7
#define MAX_BUFFER_AREA (1*1024)
#define HW_FIFO_SIZE 16


int base_addr = 0x03f8,irq_no = 4;
int kfifo_size1 = 1024*1024;

//module_param(base_addr,int,S_IRUGO);
//module_param(irq_no,int,S_IRUGO);
//module_param(kfifo_size,int,S_IRUGO);

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


irqreturn_t pseudo_serial_intr_handler(int , void *);

static int plp_kmem_open(struct inode *inode, struct file *filp)
{


	P_SERIAL_DEV *dev;
	int ret;
	dev = container_of(inode->i_cdev,P_SERIAL_DEV, cdev);
	filp->private_data = dev;
		printk(" >>>>>>>>>IN OPEN MODE\n");
//	dump_stack(); 

	outb (0xc7, base_addr + 2); //FCR enable fifo + trigger level set
	outb (0x80, base_addr + 3); //LCR 7 set DLAB
	outb (0x00, base_addr + 1); //DLM : baud rate higher byte for baud rate 9600
	outb (0x0c, base_addr + 0); //DLL : baud rate lower byte for baud rate 9600
	outb (0x03, base_addr + 3); //set 8-bit data + set no parity + 1 stop bit + disable DLAB bit
	outb (0x01, base_addr + 1); //IER ,enabling receive buffer interrupt ERBFI
	
	
	ret=request_irq(irq_no,pseudo_serial_intr_handler,0,"custom_serial_device0",dev);
	if(ret != 0)
	{
		printk(KERN_ALERT "Unable to Register Interrupt for Device\n");
                return -EBUSY;
        }
	outb (0x08, base_addr + 4);	// MCR : interrupt enable

	return 0;
}


static int plp_kmem_release(struct inode *inode, struct file *filp)
{

       P_SERIAL_DEV *dev;
       dev = filp->private_data; 


	printk(">>>>>>>>plp_kmem: device closed.\n");

	outb (0x00, base_addr + 4);	// MCR : interrupt disable
	outb (0x00, base_addr + 1);	// IER : Rx and Tx interrupt disable

	while(in_interrupt());
        	free_irq(irq_no, dev);
      	return 0;
}



static ssize_t plp_kmem_read(struct file *filp, char __user *buf,size_t count, loff_t *pos)
{
	P_SERIAL_DEV *dev= filp->private_data;
	int bytes;
	printk (">>>>>>>>>>>In read call\n");
	 

	if (kfifo_is_empty(&(dev->read_kfifo)))
	{
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(dev->read_queue, !kfifo_is_empty(&(dev->read_kfifo))) ;
	}

	if (access_ok (VERIFY_WRITE, (void __user *) buf, (unsigned long) count))
	{
		bytes = kfifo_out_locked(&(dev->read_kfifo),(void __user *) buf,count,&(dev->rd_spinlock));
		return bytes;
	}
	else
		return -EFAULT;
}




static ssize_t plp_kmem_write(struct file *filp, const char __user *buf,size_t count, loff_t *pos)
{
	P_SERIAL_DEV *dev= filp->private_data;
	int bytes;
	printk (">>>>>>>>>In write call\n");
	 
	if (kfifo_is_full(&(dev->write_kfifo)))
	{
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(dev->write_queue, !kfifo_is_full(&(dev->write_kfifo))) ;
	}


	if (access_ok (VERIFY_READ, (void __user *) buf, (unsigned long) count))
	{
		bytes = kfifo_in_locked(&(dev->write_kfifo),(void __user *) buf,count,&(dev->wr_spinlock));
			outb (0x03, base_addr + 1); //IER ,enabling Tx & Rx buffer interrupt ERBFI & EXBFI

		return bytes;
	}
	else
		return -EFAULT;
}



static struct file_operations p_device_fops = {
	.owner		= THIS_MODULE,
	.read		= plp_kmem_read,
	.write		= plp_kmem_write,
	.open		= plp_kmem_open,
	.release	= plp_kmem_release
	
};


unsigned long received=0,dropped=0;

irqreturn_t pseudo_serial_intr_handler(int irq_no, void *p_dev)
{
	int ret_val,i,j;
	irqreturn_t irq_r_flag=0;
	P_SERIAL_DEV *dev = (P_SERIAL_DEV *)p_dev;
	char lk_buff[HW_FIFO_SIZE];
	
	irq_r_flag |= IRQ_NONE;

printk("\n\n>in irqreturn_t...!\n");
	if(inb(base_addr+0x05) & 0x20) 		//comparing for Tx interrupt
	{
	printk("\n>in irqreturn_t    Tx interrupt...!\n");

		outb (0x01, base_addr + 1); //IER ,disabling Tx & enabling Rx buffer interrupt ERBFI & EXBFI
		
		//copying the data into local buffer from the kfifo buffer

		ret_val = kfifo_out_locked(&(dev->write_kfifo),lk_buff,HW_FIFO_SIZE, &(dev->wr_spinlock));

		printk("\n>in irqreturn_t    Tx interrupt   ret_val = kfifo_out_locked  %d...!\n", ret_val);
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

	printk("\n>in irqreturn_t    Rx interrupt...!\n");

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



P_SERIAL_DEV *my_dev;
static dev_t p_device_id;

static int __init p_serial_init(void)
{

	printk("1>alloc_chrdev_region(...!\n");

	if (alloc_chrdev_region(&p_device_id, 0,1, "pseudo_serial_driver"))
	{
		printk("Error in allocating the serial device driver region..!\n");
		
		return -EINVAL;
	}

	printk("2>request_region(...!\n");

	rs = request_region(base_addr,NO_OF_PORTS,"pseudo_serial_device0");	
        if(rs==NULL)
        { 
		unregister_chrdev_region(p_device_id, 1);

		
		return -EBUSY;
	}


	printk("3>my_dev = kzalloc...!\n");
	my_dev = kmalloc(sizeof(P_SERIAL_DEV),GFP_KERNEL);
        if(my_dev==NULL)
        { 
		printk("Error in creating a private object...!\n");
		unregister_chrdev_region(p_device_id, 1);
		release_region(base_addr,NO_OF_PORTS);  
		return -EBUSY;
	}



	printk("4>my_dev->read_buff = kzalloc(...!\n");
	my_dev->read_buff = kmalloc(MAX_BUFFER_AREA,GFP_KERNEL);
        if(my_dev->read_buff==NULL)
        {
	      printk("error in read_buff's memory allocation...\n");
		unregister_chrdev_region(p_device_id, 1);

	      kfree(my_dev); 
	      release_region(base_addr,NO_OF_PORTS);  
	      return -ENOMEM;
	}
	

	printk("5>kfifo_init(&(my_dev->read_kfifo)...!\n");

	kfifo_init(&(my_dev->read_kfifo), my_dev->read_buff, (unsigned int) MAX_BUFFER_AREA);


	printk("6>my_dev->write_buff = kzalloc(...!\n");
	
	my_dev->write_buff = kmalloc(MAX_BUFFER_AREA,GFP_KERNEL);
        if(my_dev->write_buff==NULL)
	{
	      printk("error in read_buff's memory allocation...\n");
		unregister_chrdev_region(p_device_id, 1);

	      kfifo_free(&(my_dev->read_kfifo));
	      kfree(my_dev); 
	      release_region(base_addr,NO_OF_PORTS);  
	      return -ENOMEM;
	}
	
	printk("7>kfifo_init(&(my_dev->write_kfifo)...!\n");

	kfifo_init(&(my_dev->write_kfifo), my_dev->read_buff, (unsigned int) MAX_BUFFER_AREA);

	printk("8>spin_lock...!\n");

	spin_lock_init(&(my_dev->wr_spinlock));
	spin_lock_init(&(my_dev->rd_spinlock));


	printk("9>init_waitqueue...!\n");

	init_waitqueue_head(&(my_dev->read_queue));
	init_waitqueue_head(&(my_dev->write_queue));

	

	printk("10>Device ID is %u (MAJOR:%d,Minor:%d)\n",p_device_id,MAJOR(p_device_id),MINOR(p_device_id));	

	cdev_init(&my_dev->cdev,&p_device_fops);


	printk("11>kobject_set_name(...!\n");

	kobject_set_name(&(my_dev->cdev.kobj),"p_serial_dev0");

printk("12>:  my_dev->cdev.ops\n");
		my_dev->cdev.ops = &p_device_fops;
		
printk("13>:  cdev_add(\n");
    
	if ((cdev_add(&my_dev->cdev, p_device_id , 1)) < 0)
	{
		printk("Error in adding the serial device to the driver region..!\n");
//		kobject_put(&(my_dev->cdev.kobj));
		unregister_chrdev_region(p_device_id, 1);
              	kfifo_free(&(my_dev->read_kfifo));
              	kfifo_free(&(my_dev->write_kfifo));
		kfree(my_dev);
		release_region(base_addr,NO_OF_PORTS);
		return -EINVAL;
	}
	return 0;

	
}



static void __exit p_serial_exit(void)
{

	cdev_del(&(my_dev->cdev));

//	kobject_put(&(my_dev->cdev.kobj));

//	kfree(my_dev->read_buff);
//	kfree(my_dev->write_buff);
	kfifo_free(&(my_dev->read_kfifo));
	kfifo_free(&(my_dev->write_kfifo));
        
	kfree(my_dev); 
	unregister_chrdev_region(p_device_id,1);
	
	release_region(base_addr,NO_OF_PORTS);

	printk("plp_kmem: unloaded...!\n");
}

module_init(p_serial_init);
module_exit(p_serial_exit);

MODULE_LICENSE("GPL"); 
