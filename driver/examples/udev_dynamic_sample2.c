//integrate this code with the other drivers, with appropriate customization !!!


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define CUSTOM_DEV_NAME "customdev"

static dev_t first;
static unsigned int count = 2;
static struct cdev *my_cdev;

static struct class *foo_class;


static int __init my_init(void)
{
        alloc_chrdev_region(&first, 0, count, CUSTOM_DEV_NAME);

	foo_class = class_create(THIS_MODULE, "custom");
	device_create(foo_class, NULL, first, NULL, "%s%d", "custom", MINOR(first));
	device_create(foo_class, NULL, first+1, NULL, "%s%d", "custom",MINOR(first+1));

	return 0;
}

static void __exit my_exit(void)
{
	device_destroy(foo_class, first);
	device_destroy(foo_class, first+1);
	class_destroy(foo_class);

	unregister_chrdev_region(first, count);
}

module_init(my_init);
module_exit(my_exit);


MODULE_LICENSE("GPL"); 
