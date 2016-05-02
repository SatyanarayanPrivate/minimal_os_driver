#include<linux/init.h>
#include<linux/sched.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/proc_fs.h>
#include<linux/slab.h>
#include<linux/gfp.h>
#include<linux/page-flags.h>
#include<linux/slab.h>



//unsigned int i=0;
static struct kmem_cache *my_slab=NULL;
struct task_struct *my_struct=NULL;
//	unsigned long page;
static int __init myslab_init(void)
{
	my_slab=kmem_cache_create("myslab_init",sizeof(struct task_struct),0,0,NULL);
	if(my_slab==NULL)
	 { 
	         printk("error in kmem_cache_create"); 
		return -ENOMEM; 
	  } 
	else
		{
			printk(KERN_ALERT"SLAB IS CREATED\n ");
		}

 	my_struct = (struct task_struct *)kmem_cache_alloc(my_slab,GFP_KERNEL);
/* if(!my_struct)
     	{
		return NULL;
	}
 
*/
	return 0;
}
static void __exit myslab_exit(void)
{
	kmem_cache_free(my_slab,my_struct);
	kmem_cache_destroy(my_slab);
	

}
module_init(myslab_init); 
module_exit(myslab_exit);

MODULE_LICENSE("GPL");
