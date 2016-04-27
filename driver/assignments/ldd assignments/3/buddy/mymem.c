#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/gfp.h>

unsigned int order = 0;		// Order of buddy allocator system
unsigned long addr;

static int __init buddy_init ( void )
{
	int N = 1, i;

	addr = __get_free_pages ( GFP_KERNEL | __GFP_DMA, order );			// Returns logical address

	if ( addr == NULL )
		printk ( KERN_ALERT "Error in allocating pages\n");

	for ( i = 0; i < order; i++ )	
	{
		N = N * 2;
	}
	printk ( KERN_ALERT "Address of allocated pages = %lu\n", addr );
	printk ( KERN_ALERT "Number of allocated pages = %d\n", N );

	printk ( KERN_ALERT "Sucessfully allocated  pages\n");
	return 0;
}


static void __exit buddy_exit ( void )
{
	// void __free_pages ( struct page *page, unsigned int order );
	
	free_pages ( addr , order );		// Frees pages
	printk ( KERN_ALERT "Successfully freed pages\n");
}

module_init ( buddy_init );
module_exit ( buddy_exit );

MODULE_LICENSE ( "GPL" );
