#include <linux/kernel.h>
#include <linux/module.h>

extern int syms2_exported;

static void __exit mymodule_exit (void) {
    printk (KERN_INFO "MODULE multiple files exit !!!!!!!!!!!!!!!!!!\n");
    syms2_exported ++;
}
 
module_exit (mymodule_exit);
 