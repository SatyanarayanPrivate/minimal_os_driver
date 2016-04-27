
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/init.h>

struct  custom_obj
{
   void * mem;    
};

struct custom_obj custom_obj1, custom_obj2;

//do not use this technique 

struct tasklet_struct  test_tasklet1 ;
struct tasklet_struct  test_tasklet2 ;

void custom_tasklet_func1(unsigned long obj)
{

   struct custom_obj *obj1 = obj;

   if(in_interrupt()) printk("...in custom tasklet func1....we are in interrupt context \n");

   obj1->mem = kmalloc(128,GFP_ATOMIC);   

}

void custom_tasklet_func2(unsigned long obj)
{
   struct custom_obj *obj1 = obj;

   if(in_interrupt()) printk("...in custom tasklet func1....we are in interrupt context \n");
   if(obj1->mem != NULL)
   { 

        kfree(obj1->mem);
   }
}

static int __init my_init (void)
{

   //here initialize tasklets 
   memset(&custom_obj1,0,sizeof(custom_obj1));
   tasklet_init(&test_tasklet1,custom_tasklet_func1,&custom_obj1);
   tasklet_init(&test_tasklet2,custom_tasklet_func2, &custom_obj1);

   tasklet_schedule (&test_tasklet1);
   tasklet_schedule (&test_tasklet2);
   return 0;
}

static void __exit my_exit (void)
{
    printk (KERN_INFO "\nHello: cleanup_module loaded at address 0x%p\n",
            cleanup_module);
    tasklet_kill(&test_tasklet1);
    tasklet_kill(&test_tasklet2);
}

module_init (my_init);
module_exit (my_exit);
