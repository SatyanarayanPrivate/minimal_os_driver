#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/proc_fs.h>
#include<linux/slab.h>
#include<linux/seq_file.h>
#include<linux/types.h>
#include<linux/sched.h>
#include<linux/mm.h>
#include<linux/highmem.h>

struct page *ret;

char  *add;

static int __init assign_init(void)
{
	ret = alloc_pages(__GFP_DMA,3);
	 
	if(ret)
	{
		printk("\npage allocated  --> %x\n", ret);
	}
	else 
	{
		printk("alloc_pages..failed....");
		return -ENOMEM;
	}

	add = kmap(ret);

	if(add)
	{
		printk("page virtual address 1--%x\n", add );
	//	printk("page virtual address 2--%x\n", (add + 4111));
		*(add + 9*4096 + 5) = "0";
	}




	return 0;
}

static void assign_exit(void)

{
	
	kunmap(ret);
	__free_pages((unsigned long int)ret,3);
	printk("i am out");
	
}

module_init(assign_init);
module_exit(assign_exit);


