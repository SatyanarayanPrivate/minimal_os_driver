#include <linux/kernel.h>
#include <linux/module.h>

int syms2_exported;

static int __init mymodule_init (void) {
    
    printk (KERN_INFO "MODULE multiple files init !!!!!!!!!!!!!!!!!!\n");
    
    return 0;
}


module_init (mymodule_init);

EXPORT_SYMBOL(syms2_exported);