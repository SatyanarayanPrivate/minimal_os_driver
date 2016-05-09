/*
 * Sample kset and ktype implementation
 *
 * Copyright (C) 2004-2007 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2007 Novell Inc.
 *
 * Released under the GPL version 2 only.
 *
 */
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include<linux/slab.h>
#include<linux/kfifo.h>
#include<linux/wait.h>
#include<linux/interrupt.h>
#include<linux/sched.h>




#define MAX_BUFFSIZE (1*1024)

struct foo_obj {
	struct kobject kobj;
	int foo;
	struct kfifo fookfifo;
	unsigned char *buff;
	wait_queue_head_t fooqueue;
	int baz;
	int bar;
};
#define to_foo_obj(x) container_of(x, struct foo_obj, kobj)


struct foo_attribute {
	struct attribute attr;
	ssize_t (*show)(struct foo_obj *foo, struct foo_attribute *attr, char *buf);
	ssize_t (*store)(struct foo_obj *foo, struct foo_attribute *attr, const char *buf, size_t count);
};
#define to_foo_attr(x) container_of(x, struct foo_attribute, attr)


static ssize_t foo_attr_show(struct kobject *kobj,
			     struct attribute *attr,
			     char *buf)
{
	struct foo_attribute *attribute;
	struct foo_obj *foo;

	attribute = to_foo_attr(attr);
	foo = to_foo_obj(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(foo, attribute, buf);
}


static ssize_t foo_attr_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
	struct foo_attribute *attribute;
	struct foo_obj *foo;

        //dump_stack();
	attribute = to_foo_attr(attr);
	foo = to_foo_obj(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(foo, attribute, buf, len);
}


static struct sysfs_ops foo_sysfs_ops = {
	.show = foo_attr_show,
	.store = foo_attr_store,
};


static void foo_release(struct kobject *kobj)
{
	struct foo_obj *foo;

	foo = to_foo_obj(kobj);

	kfifo_free(&(foo->fookfifo));
	
	kfree(foo);
}


static ssize_t foo_show(struct foo_obj *foo_obj, struct foo_attribute *attr,
			char *buf)
{
	int bytes;


	printk("foo show \n");

 
	struct kfifo *kfifooperate = &foo_obj->fookfifo;
	bytes = kfifo_len(kfifooperate);
	printk("inside read call...%d bytes from kfifo_len.... \n", bytes);
	if(bytes==0)
	{
		/*if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{*/

		printk("inside read call...going to wait queue \n");
        wait_event_interruptible(foo_obj->fooqueue,(kfifo_len(kfifooperate) != 0));
		//}
	}
   
//	if(access_ok(VERIFY_WRITE,(void __user*)buff,(unsigned long)count))
//	{
	bytes = kfifo_len(kfifooperate);
		bytes = kfifo_out(kfifooperate,(unsigned char*)buf, bytes);

		printk("inside read call...%d bytes of data read successfully.... \n", bytes);
		wake_up_interruptible(&foo_obj->fooqueue);
		return bytes;
//	}
//	else
//		return -EFAULT;


	
}

static ssize_t foo_store(struct foo_obj *foo_obj, struct foo_attribute *attr,
			 const char *buf, size_t count)
{
	int bytes;


	printk("foo store \n");

	printk("inside write call... count %d..... \n", count);
	
	struct kfifo *kfifooperate = &foo_obj->fookfifo;
	

	bytes = kfifo_avail(kfifooperate);
	printk("inside write call...%d bytes from kfifo_avail.... \n", bytes);
        if(bytes == 0)
	{
		
		/*if(file->f_flags & O_NONBLOCK)
		{
			return -EAGAIN;
		}
		else
		{*/
		printk("inside write call...going to wait queue \n");
        wait_event_interruptible(foo_obj->fooqueue,(kfifo_avail(kfifooperate) != 0));
		//}
	}
     
	if(access_ok(VERIFY_READ,(void __user*)buf,(unsigned long)count))
	{
		

		bytes = kfifo_in(kfifooperate,(unsigned char*)buf,count);

		printk("inside write call...%d bytes of data written successfully.... \n", bytes);
		wake_up_interruptible(&foo_obj->fooqueue);
		return bytes;
	}
	else
		return -EFAULT;


}


static struct foo_attribute foo_attribute =
	__ATTR(dev_param1, 0666, foo_show, foo_store);


static ssize_t b_show(struct foo_obj *foo_obj, struct foo_attribute *attr,
		      char *buf)
{
	int var;

	if (strcmp(attr->attr.name, "dev_param2") == 0)
		var = foo_obj->baz;
	else
		var = foo_obj->bar;
	return sprintf(buf, "%d\n", var);
}

static ssize_t b_store(struct foo_obj *foo_obj, struct foo_attribute *attr,
		       const char *buf, size_t count)
{
	int var;

	sscanf(buf, "%du", &var);
	if (strcmp(attr->attr.name, "dev_param2") == 0)
		foo_obj->baz = var;
	else
		foo_obj->bar = var;
	return count;
}

static struct foo_attribute baz_attribute =
	__ATTR(dev_param2, 0666, b_show, b_store);
static struct foo_attribute bar_attribute =
	__ATTR(dev_param3, 0666, b_show, b_store);


static struct attribute *foo_default_attrs[] = {
	&foo_attribute.attr,
	NULL,	
};



static struct kobj_type foo_ktype = {
	.sysfs_ops = &foo_sysfs_ops,
	.release = foo_release,
	.default_attrs = foo_default_attrs,
};

static struct kset *example_kset;
static struct foo_obj *foo_obj;
static struct foo_obj *bar_obj;
static struct foo_obj *baz_obj;

static struct foo_obj *create_foo_obj(const char *name)
{
	struct foo_obj *foo;
	int retval;

	
	foo = kzalloc(sizeof(*foo), GFP_KERNEL);
	if (!foo)
		return NULL;

	
	foo->kobj.kset = example_kset;

		foo->buff = kmalloc(MAX_BUFFSIZE,GFP_KERNEL);
		if(foo->buff == NULL)
		{
			printk("Error in allocating memory for device buffer....\n");

				kfree(foo); 
					
				return -ENOMEM;
				
		}
	
		kfifo_init(&(foo->fookfifo), (foo->buff), (unsigned int) MAX_BUFFSIZE);

		if(&(foo->fookfifo) == NULL)
		{
			printk("Error in initializing kfifo.....\n");
			
			    kfree(foo->buff);
			 
				return -ENOMEM;
			
		}

		init_waitqueue_head(&foo->fooqueue);

	
	retval = kobject_init_and_add(&foo->kobj, &foo_ktype, NULL, "%s", name);
	if (retval) {
		kfifo_free(&(foo->fookfifo)); 
		kfree(foo);
		return NULL;
	}

	
	
       kobject_uevent(&foo->kobj, KOBJ_ADD);

	return foo;
}

static void destroy_foo_obj(struct foo_obj *foo)
{
	kobject_put(&foo->kobj);
}

static int example_init(void)
{
	



	example_kset = kset_create_and_add("kset_devices_typeA2", NULL, kernel_kobj);
	if (!example_kset)
		return -ENOMEM;

	
	foo_obj = create_foo_obj("device0");
	if (!foo_obj)

	return -EINVAL;
}

static void example_exit(void)
{
	destroy_foo_obj(baz_obj);
	destroy_foo_obj(bar_obj);
	destroy_foo_obj(foo_obj);
	kset_unregister(example_kset);
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Greg Kroah-Hartman <greg@kroah.com>");
