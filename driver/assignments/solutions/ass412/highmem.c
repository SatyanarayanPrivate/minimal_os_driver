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
struct page *add2=NULL;
struct page *add3=NULL;
static int __init assign_init(void)
{
	ret = alloc_pages(__GFP_HIGHMEM,5);
	 
	if(ret)
	{
		printk("\npage allocated %x\n", (unsigned int)ret);
	}
	else 
	{
		printk("alloc_pages..failed........");
		return -ENOMEM;
	}

	add = kmap(ret);

	if(add)
	{
		printk("page virtual address %x\n", (unsigned int)add);
	}

	kunmap(ret);


	
	add2 = kmap(++ret);

	if(add2)
	{
		printk("page2virtual address %x\n", (unsigned int)add2);
	}
	
	return 0;
}

static void assign_exit(void)

{

	kunmap(ret);
	__free_pages((unsigned long int)ret,5);
	printk("i am out");
	
}

module_init(assign_init);
module_exit(assign_exit);


