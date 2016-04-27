/*                                                     
 * $Id: hello.c,v 1.5 2004/10/26 03:32:21 corbet Exp $ 
 */                                                    
//these headers are from the kernel - source tree,whose path was passed
//during make -C <ksrcpath>  M=`pwd`  modules  ??
//
#include <linux/init.h>
#include <linux/module.h>

//when kernel module is loaded using insmod <name>.ko, 
//system copies the kernel module to system space and 
//memory is allocated for code/data of the kernel module 
//
//in addition, system creates and manages a kernel module
//object to maintain a kernel module - however, do not 
//confuse a kernel module object with a pd or a td - 
//meaning, kernel module object is not a schedulable object !!!

//all such kernel module objects are maintained in the system 
//space by the system, in a list !!!

//once a kernel module is successfully loaded into system space,
//it becomes part and parcel of the monolithic kernel - it may 
//be unloaded from the monolithic kernel using rmmod !!!

//the above describes the basic architecture used for 
//loading and unloading kernel modules 

//
//below, we discuss about modules methods and their execution !!!
//
//when a module is loaded successfully into  system space,
//init method of the module is executed by the system 
//once init method returns successfully, module is resident
//in the system space, until the module is unloaded - when a 
//module is resident in the system space, it can access the methods
//of the kernel space and it can also provide methods that may 
//be accessed by other components/subsystems/modules in system space !!!
//when a module is unloaded from system space, system executes
//exit method of the module, first !!!
//
//a module is allowed to access methods and objects that are 
//exported by the other subsystems/components/modules 
//
//in addition, a module can also export methods and objects
//that may be accessed by other components/subsystems 
//
//exporting is done with a set of rules by a given module
//and other components of the kernel !!!

















//you may declare the global variables 
//the memory is allocated using non-contiguous memory allocator
//of the kernel
 static int new_symbol = 5; //this will allocated in the data region of the module 
 char *str1 = NULL;
 int *var1 = NULL;

//the routine that will be executed once when the module is added
//to the kernel space-during module loading
//typically, used to allocate resources and initialise
//hw / sw entities !!!
static int hello_init(void)
{

        int local_var1 =0;//this will be allocated on the system stack 

        dump_stack();

        //here, trivial printks are used
        //ideally, some tangible coding will be added 
    printk(KERN_ALERT "Hello, system world\n");
      
        //printk(KERN_ALERT "var1 is %d\n", *var1); 
        printk(KERN_ALERT "string is %s\n", str1); 
    return 0;
}


//this routine is called only once when the module is unloaded 
//
//
//used tofree resources allocated during init or other methods
//and may also be used to shutdown hw controllers and destroy
//lists/objects 
static void hello_exit(void)
{
        //any local variable will allocated on the kernel stack 

        //this may be used to free some of the dynamically allocated
        //resources during the init() function above

        dump_stack();

    printk(KERN_ALERT "Goodbye, cruel world\n");
}

//refer to page nos 31-32
//module_xxx() is mandatory 
//these macros add special info. in a special section of the module's
//object file

module_init(hello_init);
module_exit(hello_exit);

//you may export symbols from a module - like you do in the kernel 
//symbols exported from a module are stored in the module's symbol 
//table in the kernel-space


//all symbols exported via EXPORT_SYMBOL_GPL() are added to the 
//GPL exported symbol table of this kernel module -
//any  kernel module that complies with GPL license may 
//import symbols from a GPL exported symbol table !!! if
//a module does not comply with GPL license, it cannot import
//symbols from a GPL exported symbol table !!!


//EXPORT_SYMBOL_GPL(new_symbol);

//all symbols exported via EXPORT_SYMBOL() are added to the 
//regular exported symbol table of this kernel module -
//any other kernel module may import symbols from a regular
//exported symbol table !!! 

EXPORT_SYMBOL(new_symbol);

//whenever a kernel module is loaded into system space and 
//it is importing symbols, it is the responsibility of the
//module related loader of the kernel to validate requested 
//symbols against exported symbols of appropriate symbol
//tables - if all requested symbols can be satisfied, 
//system will allow the respective kernel module to be 
//loaded !!! if one or more requested symbols cannot be 
//satisfied, system will disallow the loading of the respective
//kernel module !!!



//we must use a license - it may be GPL or proprietary
//if we use GPL license, all exported methods/objects are
//available for access - otherwise, only a subset of 
//exported methods/objects will be available for usage !!!

//MODULE_LICENSE("Dual BSD/GPL");
MODULE_LICENSE("GPL");











