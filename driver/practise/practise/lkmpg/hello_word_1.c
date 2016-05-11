#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#define DRIVER_AUTHOR "sample auther name"
#define DRIVER_DESC "sample description"

static int __init my_module_init (void) {
    
    printk ( KERN_INFO "\nhello word init !!!!!!!!!!!!!!!!!!11\n");
    
    return 0;
}

static void __exit my_module_cleanup (void) {
    printk ( KERN_INFO "\nhello word exit !!!!!!!!!!!!!!!!!!11\n");
}


module_init (my_module_init);
module_exit (my_module_cleanup);


/*
* You can use strings, like this:
*/
/*
* Get rid of taint message by declaring code as GPL.
*/
MODULE_LICENSE("GPL");
/*
* Or with defines, like this:
*/
MODULE_AUTHOR(DRIVER_AUTHOR);
/* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);
/* What does this module do */
/*
* This module uses /dev/testdevice. The MODULE_SUPPORTED_DEVICE macro might
* be used in the future to help automatic configuration of modules, but is
* currently unused other than for documen
*/
MODULE_SUPPORTED_DEVICE("testdevice");