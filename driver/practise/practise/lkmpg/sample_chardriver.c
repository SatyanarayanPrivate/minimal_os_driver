#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>


#define DEVICE_NAME "chardevice"
#define DEVICE_BUFFER_SIZE 30


static int major_num = 0x00;
static int chrdevice_busy = 0x00;
static char *chrdevice_buffer = NULL;
static int chrdevice_buffer_offset = 0x00;


static void chrdevice_fetch_data (void) {
    static int chrdevice_read_count = 0x01;
    sprintf (chrdevice_buffer, "chardevice read count:: %04d", chrdevice_read_count);
    printk (KERN_INFO "chardevice read count:: %04d", chrdevice_read_count);
    chrdevice_read_count ++;
}

static int chrdevice_open (struct inode *node, struct file *file) {
    
    if (chrdevice_busy) {
        printk (KERN_ALERT "chardevice error:: device already busy\n");
        return -EBUSY;
    }
    chrdevice_busy ++;
    
    printk (KERN_ALERT "chardevice :: device accessed\n");
    return 0;
}

static ssize_t chrdevice_read (struct file *file, char *usr_buffer, size_t usr_buffer_size, loff_t *offset) {
    ssize_t bytes_read = 0x00;
    
    if (usr_buffer_size == 0x00) {
        return 0x00;
    }
    
    while (usr_buffer_size) {
 
        if (chrdevice_buffer_offset >= DEVICE_BUFFER_SIZE) {
            chrdevice_buffer_offset = 0x00;
            chrdevice_fetch_data ();
        }

        // chardevice content to user space
        put_user (chrdevice_buffer[chrdevice_buffer_offset], usr_buffer ++);
        chrdevice_buffer_offset ++;
        printk (KERN_INFO "usr_buffer_size:: %d chrdevice_buffer_offset:: %d\n", usr_buffer_size, chrdevice_buffer_offset);
        usr_buffer_size --;
        bytes_read ++;
    }
    
    return bytes_read;
}

static int chrdevice_release (struct inode *node, struct file *file) {
    
    chrdevice_busy --;
    
    printk (KERN_ALERT "chardevice :: device released\n");
    
    return 0;
}

static struct file_operations chrdevice_fops = {
    .open = chrdevice_open,
    .read = chrdevice_read,
//     .write = chrdevice_write,
    .release = chrdevice_release,    
};

static int chrdrive_init (void) {
    
    major_num = register_chrdev (0, DEVICE_NAME, &chrdevice_fops);
    
    if (major_num < 0) {
        printk (KERN_ALERT "chardevice register error:: %d\n", major_num);
        return major_num;
    }
    printk (KERN_INFO "chardevice:: \"%s\" registered with major number:: %d\n", DEVICE_NAME, major_num);
    printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, major_num);
    printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
    printk(KERN_INFO "the device file.\n");
    printk(KERN_INFO "Remove the device file and module when done.\n");
   
    chrdevice_buffer = kmalloc (sizeof(DEVICE_BUFFER_SIZE), GFP_KERNEL);
    if (chrdevice_buffer == NULL) {
        printk (KERN_ALERT "chardevice :: memory error: kmalloc failed\n");
        return -ENOMEM;
    }
    
    chrdevice_fetch_data ();
    chrdevice_buffer_offset = 0x00;
    
    return 0;
}

static void chrdevice_exit (void) {
    
    if (chrdevice_buffer) {
        kfree (chrdevice_buffer);
    }
    
    unregister_chrdev (major_num, DEVICE_NAME);
    printk (KERN_INFO "chardevice:: \"%s\" unregistered\n", DEVICE_NAME);
}

module_init (chrdrive_init);
module_exit (chrdevice_exit);

