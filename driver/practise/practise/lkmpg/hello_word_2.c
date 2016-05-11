#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

static int __init my_module_init (void) {
    
    printk ( KERN_INFO "\nhello word init !!!!!!!!!!!!!!!!!!11\n");
    
    return 0;
}

static void __exit my_module_cleanup (void) {
    printk ( KERN_INFO "\nhello word exit !!!!!!!!!!!!!!!!!!11\n");
}


module_init (my_module_init);
module_exit (my_module_cleanup);