#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/blkdev.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#define SERIAL_NO_REGISTERS 8 
#define HW_FIFO_SIZE 16   
#define MY_BUFSIZE (1024*1024)

int base_addr,irq_no,kfifo_size1;
int ndevice = 1;

static dev_t dev;
static struct class *pseudo_class;
//static struct cdev my_cdev;
//static char *my_uart_buffer;

typedef struct p_serial_dev{
	struct list_head list;
	struct cdev cdev;
	int base_addr;
	int irq_no;
	int kfifo_size1;
	struct kfifo write_kfifo;
	struct kfifo read_kfifo;
	spinlock_t spinlock1;
	spinlock_t spinlock2;
	unsigned char *read_buff;
	unsigned char *write_buff;
	wait_queue_head_t read_queue;
	wait_queue_head_t write_queue;
	struct tasklet_struct tx;
	struct tasklet_struct rx;
}P_SERIAL_DEV;




LIST_HEAD(dev_list);



/*void tasklet_rx(unsigned long private_obj)
{
//	short int i=0,bytes;
	P_SERIAL_DEV *dev = (P_SERIAL_DEV *) private_obj;
	wake_up_interruptible (&(dev->read_queue));
} 

void tasklet_tx(unsigned long private_obj)
{
//	short int i=0,bytes;
	P_SERIAL_DEV *dev = (P_SERIAL_DEV *) private_obj;
	wake_up_interruptible (&(dev->write_queue));
}
*/
irqreturn_t serial_intr_handler(int irq_no ,void *dev)
{
	int ret_val,i,j;
	irqreturn_t irq_r_flag = 0;
	P_SERIAL_DEV *dev1 = dev;
//	char byte;
	char lk_buff[MY_BUFSIZE];

	irq_r_flag |= IRQ_NONE ;


	if(inb(dev1->base_addr +0x05) & 0x20 )
	{
		if(ret_val!= 0)
		{
			for(i=0;i<ret_val; i++)
				outb(lk_buff[i],dev1->base_addr+0x0);
		}
		
		wake_up_interruptible (&(dev1->write_queue));
		irq_r_flag |= IRQ_HANDLED;

	}

if(inb(dev1->base_addr+0x05)&0x01)
{
  for(j=0;;j++)  
  {
    if(inb(dev1->base_addr+0x05)&0x01)
    {

	lk_buff[j] = inb(dev1->base_addr+0x0);
    }

    else break;
  }
	kfifo_out(&(dev1->read_kfifo),lk_buff,j);
//	tasklet_schedule(&(dev1->rx));
  
	wake_up_interruptible (&(dev1->read_queue));
	  irq_r_flag |= IRQ_HANDLED; 
}

  return irq_r_flag; 
}

static int my_open(struct inode *inode, struct file *file)
{
	P_SERIAL_DEV *dev;
	int ret;

	dev = container_of(inode->i_cdev,P_SERIAL_DEV, cdev);
	file->private_data = dev;

	printk(KERN_ALERT "IN OPEN MODE\n");
	outb(0x80,dev->base_addr +3);
	outb (0x00,dev->base_addr + 1);
	outb (0x0c,dev->base_addr + 0);
	outb (0x03, dev->base_addr + 3);
	outb (0x01, dev->base_addr + 1);
	outb (0xc7,dev->base_addr + 2);
	ret=request_irq(irq_no,serial_intr_handler, IRQF_DISABLED|IRQF_SHARED,"serial_device0",dev);
	if(ret != 0)
	{
			printk(KERN_ALERT "Unable to Register Interrupt MY_BUFSIZE (1024*1024)for Device\n");
                        return -EBUSY;
	}
	
	outb(0x08,dev->base_addr+4);
	printk(KERN_DEBUG"my:opendevice\n");
	return 0;

}


static int my_release(struct inode *inode ,struct file *file)
{
	P_SERIAL_DEV *dev;
	dev = file->private_data;

#ifdef PLP_DEBUG
	printkl(KERN_DEBUG"my:device close");
#endif

	outb(0x00,dev->base_addr +4);
	free_irq(dev->irq_no,dev);

	return 0;
}


static ssize_t my_read(struct file *file, char __user *buf,size_t count, loff_t *ppos)
{
	P_SERIAL_DEV *dev =file->private_data;
	int bytes;
	printk("read call\n");

	bytes = kfifo_len(&(dev->read_kfifo));

	if((bytes) == 0)
	{
		if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{
			wait_event_interruptible(dev->read_queue, kfifo_len(&(dev->read_kfifo)) != 0);
		}
	}
	if(access_ok(VERIFY_WRITE,(void __user *)buf, (unsigned long)count))
	{
		bytes = kfifo_in(&(dev->read_kfifo),buf,count);
		return bytes;
	}
	else
		return -EFAULT;
}



static ssize_t my_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
        

	unsigned int  ret_val;
//	unsigned char ch;
          P_SERIAL_DEV *dev = file->private_data ;  

	outb (0x03, dev->base_addr + 1);

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
	struct resource *rs1;
	P_SERIAL_DEV *my_dev;

	
	module_param(base_addr,int,S_IRUGO);
	
	module_param(irq_no,int,S_IRUGO);
	
//	module_param(kfifo_size1,int,S_IRUGO);


static struct file_operations my_fops = {
	.read = my_read,
	.write = my_write,
	.open  = my_open,
	.release = my_release,
	.owner = THIS_MODULE,
};
static int __init my_serial_init(void)
{
//	int ret;
	//P_SERIAL_DEV *dev;
	my_dev = kzalloc(sizeof(P_SERIAL_DEV),GFP_KERNEL);
	rs1 = request_region(my_dev->base_addr,1,"custom_serial_device0");	
        if(rs1==NULL){ 
		kfree(my_dev);
		return -EBUSY;
	}
	
	list_add_tail(&my_dev->list,&dev_list);
	my_dev->write_buff = kmalloc(MY_BUFSIZE,GFP_KERNEL);
	
	if(my_dev->write_buff==NULL)
	{
		printk("error in write_buff's kmalloc()...\n");
		kfree(my_dev);
		release_region(base_addr,SERIAL_NO_REGISTERS);
		return -ENOMEM;
	}




	my_dev->read_buff = kmalloc(MY_BUFSIZE,GFP_KERNEL);
	if(my_dev->read_buff==NULL)
	{
		printk("error in read_buff's kmalloc()...\n");
		kfree(my_dev);
		kfree(my_dev->write_buff);
		release_region(my_dev->base_addr,SERIAL_NO_REGISTERS);
		return -ENOMEM;
	}
	spin_lock_init(&(my_dev->spinlock1));
	spin_lock_init(&(my_dev->spinlock2));
	 kfifo_init (&(my_dev->write_kfifo),my_dev->write_buff, MY_BUFSIZE);
	if(&(my_dev->write_kfifo)==NULL)
	{
		printk("error in write_kfifo_init...\n");
		kfree(my_dev->write_buff);
		kfree(my_dev->read_buff);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_REGISTERS);
		return 0;
	}
	 kfifo_init (&(my_dev->read_kfifo),my_dev->read_buff, MY_BUFSIZE);
	if(&(my_dev->read_kfifo)==NULL)
	{
		printk("error in read_kfifo_init...\n");
		kfifo_free(&my_dev->write_kfifo);
		kfree(my_dev->write_buff);
		kfree(my_dev->read_buff);
		kfree(my_dev);
		release_region(base_addr,SERIAL_NO_REGISTERS);
		return 0;
	}
	init_waitqueue_head(&my_dev->write_queue);
	init_waitqueue_head(&my_dev->read_queue);
	
	
//	tasklet_init(&(my_dev->tx), tasklet_tx, &my_dev); 
  //  	tasklet_init(&(my_dev->rx), tasklet_rx, &my_dev); 

	if(alloc_chrdev_region(&dev, 0,1, "serial_driver"))
		goto error;

	cdev_init(&my_dev->cdev,&my_fops);

	kobject_set_name(&(my_dev->cdev.kobj),"serial_dev0");

	my_dev->cdev.ops = &my_fops;

	if (cdev_add(&my_dev->cdev, dev, 1)) 
	{
		kobject_put(&(my_dev->cdev.kobj));
		unregister_chrdev_region(dev,1);
		kfifo_free(&my_dev->write_kfifo);
		kfifo_free(&my_dev->read_kfifo);
		kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_REGISTERS);
		goto error;

	}
	pseudo_class = class_create(THIS_MODULE, "serial_class");
	if (IS_ERR(pseudo_class)) {
		printk(KERN_ERR "dev1: Error creating class.\n");
		device_destroy(pseudo_class,dev);
		cdev_del(&(my_dev->cdev));
		unregister_chrdev_region(dev,1);
		kfifo_free(&my_dev->write_kfifo);
		kfifo_free(&my_dev->read_kfifo);
                kfree(my_dev);
		release_region(my_dev->base_addr,SERIAL_NO_REGISTERS);
		goto error;
	}

	device_create(pseudo_class, NULL,dev, NULL, "custom_serial_dev0");

	printk(KERN_INFO "plp_kmem: loaded.\n");

	return 0;
error:
	printk(KERN_ERR "plp_kmem: cannot register device.\n");
	return -EINVAL;
}

static void __exit my_serial_exit(void)
{


	device_destroy(pseudo_class, dev);
	class_destroy(pseudo_class);

       
	cdev_del(&(my_dev->cdev));
	unregister_chrdev_region(dev,1);
	kfifo_free(&my_dev->write_kfifo);
	kfifo_free(&my_dev->read_kfifo);
	kfree(my_dev); 
	release_region(base_addr,SERIAL_NO_REGISTERS);
		
	printk(KERN_INFO "plp_kmem: unloading.\n");
}



module_init(my_serial_init);
module_exit(my_serial_exit);
		
