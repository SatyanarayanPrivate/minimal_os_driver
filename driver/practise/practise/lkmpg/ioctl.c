#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "myioctl.h"

#define BUFF_LEN 128

static int device_open_flag = 0x0;

static char message_buffer[BUFF_LEN] = {0};

static char *message_ptr;

static int device_open (struct inode *inode, struct file *file) {
    
    printk (KERN_INFO "device_open:: %p with inode:: %p", file, inode); 
    
    if (device_open_flag) {
        return -EBUSY;
    }
    
    device_open_flag ++;
    message_ptr = message_buffer;
    return 0;
}

static int device_release (struct inode *inode, struct file *file) {
    printk (KERN_INFO "device_open:: %p with inode:: %p", file, inode); 
    device_open_flag --;
    return 0;
}

static ssize_t device_read (struct file *file, char *usr_buffer, size_t usr_len, loff_t *offset) {
    ssize_t bytes_read = 0x00;
    
    printk (KERN_INFO "device_read(%p,%p,%d)\n", file, usr_buffer, usr_len);
    
    if (*message_ptr == 0x00) {
        return 0;
    }
    
    while (usr_len && *message_ptr) {
        put_user (*(message_ptr ++), usr_buffer ++);
        bytes_read ++;
        usr_len --;
    }
    printk (KERN_INFO "Read %d bytes, %d left\n", bytes_read, usr_len);
    
    return bytes_read;
}


static ssize_t device_write (struct file *file, const char *usr_buffer, size_t usr_len, loff_t *offset) {
    int counter = 0x00;
    
    for (counter = 0x00; counter < usr_len; counter ++) {
        get_user (message_buffer[counter], usr_buffer + counter);
    }
    
    message_ptr = message_buffer;
    
    return counter;
}

long device_ioctl (struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
    
    long ret_value = 0x00;
    
    switch (ioctl_num) {
        case IOCTL_SET_MSG: {
            char *traverse;
            char ch;
            int byte_counter = 0x00;
            
            traverse = (char *) ioctl_param;
            get_user (ch, traverse);
            for (byte_counter = 0x00; ch && (byte_counter < BUFF_LEN) ; byte_counter ++) {
                get_user (ch, traverse);
                traverse ++;
            }
            ret_value = device_write (file, (char *) ioctl_param, byte_counter, 0x00);
            
            return ret_value;
        }
        
        case IOCTL_GET_MSG: {
            
            ret_value = device_read (file, (char *) ioctl_param, 99, 0);
            
            put_user ('\0', (char *) ioctl_param + ret_value);
            
            return ret_value;   
        }
        
        case IOCTL_GET_NTH_BYTE: {
            return message_buffer [ioctl_param];
        }
        
        default:
            return -1;
    }
}

static struct file_operations device_operations = {
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};

static int device_init (void) {
    int ret_val = 0x00;
    
    ret_val = register_chrdev (MAJOR_NUM, DEV_NAME, &device_operations);
    
    if (ret_val < 0) {
        printk (KERN_ALERT "%s failed with %d\n","Sorry, registering the character device ", ret_val);
        return ret_val;    
    }
    
    printk (KERN_INFO "%s The major device number is %d.\n","Registeration is a success", MAJOR_NUM);
    printk (KERN_INFO "If you want to talk to the device driver,\n");
    printk (KERN_INFO "you'll have to create a device file. \n");
    printk (KERN_INFO "We suggest you use:\n");
    printk (KERN_INFO "mknod %s c %d 0\n", DEV_NAME, MAJOR_NUM);
    printk (KERN_INFO "The device file name is important, because\n");
    printk (KERN_INFO "the ioctl program assumes that's the\n");
    printk (KERN_INFO "file you'll use.\n");
    
    return 0;
}


static void device_exit (void) {
    /*
    * Unregister the device
    */
    unregister_chrdev (MAJOR_NUM, DEV_NAME);
    
    printk (KERN_INFO "device \"%s\" unregistered\n", DEV_NAME);
}

module_init (device_init);
module_exit (device_exit);



    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    