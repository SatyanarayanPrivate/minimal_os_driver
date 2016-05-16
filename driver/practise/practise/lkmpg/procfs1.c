
// PLease Note this example gives compilation error






#include <linux/kernel.h>          /* We're doing kernel work */
#include <linux/module.h>          /* Specifically, a module */
#include <linux/proc_fs.h>         /* Necessary because we use proc fs */
#include <asm/uaccess.h>           /* for copy_*_user */
#include <linux/sched.h>

#define PROC_ENTRY_FILENAME "mybuffer2k"
#define PROCFS_MAX_SIZE     2048

/**
* The buffer (2k) for this module
*
*/
static char myprocfs_buffer[PROCFS_MAX_SIZE];

/**
* The size of the data hold in the buffer
*
*/
static unsigned long myprocfs_buffer_size = 0;

/**
* The structure keeping information about the /proc file
*
*/
static struct proc_dir_entry *myproc_file = NULL;

/**
* This funtion is called when the /proc file is read
*
*/
static ssize_t myproc_read (struct file *flip, char *usr_buffer, size_t usr_length, loff_t *offset) {
    static int finished = 0;
    
    /*
    * We return 0 to indicate end of file, that we have
    * no more information. Otherwise, processes will
    * continue to read from us in an endless loop.
    */
    if (finished ) {
        printk(KERN_INFO "procfs_read: END\n");
        finished = 0;
        return 0;
    }
    
    finished = 1;

    /*
    * We use put_to_user to copy the string from the kernel's
    * memory segment to the memory segment of the process
    * that called us. get_from_user, BTW, is
    * used for the reverse.
    */
    if (copy_to_user (usr_buffer, myprocfs_buffer, myprocfs_buffer_size)) {
        return -EFAULT;
    }
    
    printk(KERN_INFO "procfs_read: read %lu bytes\n", myprocfs_buffer_size);
    return myprocfs_buffer_size;
    /* Return the number of bytes "read" */
    
}

/*
* This function is called when /proc is written
*/
static ssize_t myproc_write (struct file *file, const char *usr_buffer, size_t usr_length, loff_t * offset) {
    
    if (usr_length > PROCFS_MAX_SIZE ) {
        myprocfs_buffer_size = PROCFS_MAX_SIZE;
    }
    else {
        myprocfs_buffer_size = usr_length;
    }
    
    if ( copy_from_user(myprocfs_buffer, usr_buffer, myprocfs_buffer_size) ) {
        return -EFAULT;
    }
    printk(KERN_INFO "procfs_write: write %lu bytes\n", myprocfs_buffer_size);
    return myprocfs_buffer_size;
}

/*
 * This function decides whether to allow an operation (return zero) or not allow it (return a non-zero which indicates why it is not allowed).
*
* The operation can be one of the following values:
* 0 − Execute (run the "file" − meaningless in our case)
* 2 − Write (input to the kernel module)
* 4 − Read (output from the kernel module)
*
* This is the real function that checks file permissions. The permissions returned by ls −l are for referece only, and can be overridden here.
*/
static int mymodule_permission (struct inode *inode, int op, struct nameidata *foo) {
    /*
    * We allow everybody to read from our module, but
    * only root (uid 0) may write to it
    */
    if ((op == 4) || (op == 2 && current->euid == 0)) {
        return 0;
    }
    /*
    * If it's anything else, access is denied
    */
    return -EACCES;
}

/*
* The file is opened − we don't really care about
* that, but it does mean we need to increment the
* module's reference count.
*/
int myproc_open(struct inode *inode, struct file *file)
{
    try_module_get(THIS_MODULE);
    return 0;
}

/*
* The file is closed − again, interesting only because
* of the reference count.
*/
int myproc_close(struct inode *inode, struct file *file)
{
    module_put(THIS_MODULE);
    return 0;
    /* success */
}

static struct file_operations myproc_fops = {
    .read = myproc_read,
    .write = myproc_write,
    .open = myproc_open,
    .release = myproc_close,
};

/*
* Inode operations for our proc file. We need it so
* we'll have some place to specify the file operations
* structure we want to use, and the function we use for
* permissions. It's also possible to specify functions
* to be called for anything else which could be done to
* an inode (although we don't bother, we just put
* NULL).
*/
static struct inode_operations myproc_inode_ops = {
    .permission = mymodule_permission,
/* check for permissions */
};

/*
* Module initialization and cleanup
*/
int init_module (void) {
    /* create the /proc file */
    
    // static inline struct proc_dir_entry *proc_create(const char *name, umode_t mode, struct proc_dir_entry *parent, 
    //                                                  const struct file_operations *proc_fops)
    myproc_file = proc_create (PROC_ENTRY_FILENAME, 0644, NULL, &myproc_fops);
    /* check if the /proc file was created successfuly */
    if (myproc_file == NULL){
        printk(KERN_ALERT "Error: Could not initialize /proc/%s\n", PROC_ENTRY_FILENAME);
        return -ENOMEM;
    }
    myproc_file->owner = THIS_MODULE;
    myproc_file->proc_iops = &myproc_inode_ops;
    myproc_file->proc_fops = &myproc_fops;
    myproc_file->mode = S_IFREG | S_IRUGO | S_IWUSR;
    myproc_file->uid = 0;
    myproc_file->gid = 0;
    myproc_file->size = 80;
    printk(KERN_INFO "/proc/%s created\n", PROC_ENTRY_FILENAME);
    return 0;
    /* success */
}
void cleanup_module()
{
    remove_proc_entry(PROC_ENTRY_FILENAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROC_ENTRY_FILENAME);
}
