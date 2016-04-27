#include <linux/init.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/highmem.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/seq_file.h>

struct page *pg;
unsigned long *addr;
unsigned int order = 1;
struct page * pg;

static int __init buddy_init ( void )
{
	pg = alloc_pages ( __GFP_HIGHMEM | GFP_KERNEL, order );

	addr = ( unsigned long * ) kmap ( pg );	// Allocated pages in highmem and return its address

	if ( pg == NULL )
		printk ( KERN_ALERT "Fail to allocate to pages\n");
	else
		printk ( KERN_ALERT "Suceessfully allocated pages at address %lu\n", *addr );

	return 0;
}

static void __exit buddy_exit ( void )
{
	kunmap ( pg );
	__free_pages ( pg, order );
	printk ( KERN_ALERT "Buddy exited\n");
}

module_init ( buddy_init );
module_exit ( buddy_exit );

MODULE_LICENSE ("GPL");

// Check # cat /proc/buddyinfo 

