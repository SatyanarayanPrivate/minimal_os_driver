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

struct page *ret=NULL;
struct page *add=NULL;
static int __init assign_init(void)
{
	ret = alloc_pages(__GFP_DMA,5);
	 add = kmap(ret);
	if(ret)
	{
		printk("\npage allocated\n");
	}
	else 
		printk("gmg..fail..allocated....");
	if(add)
	{
		printk("page virtual address\n");
	}

	return 0;
}

static void assign_exit(void)

{
	unsigned int  i=0;
	i = page_address(ret);
	printk("address_of_page is-->%x\n",i);
	kunmap(ret);
	free_pages((unsigned long int)ret,5);
	printk("i am out");
	
}

module_init(assign_init);
module_exit(assign_exit);


