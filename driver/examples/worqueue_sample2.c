#include <linux/module.h>
#include <linux/fs.h>
#include <linux/workqueue.h>


static struct workqueue_struct *my_workq;

static struct my_dat {
	atomic_t len;
	struct work_struct work;
} my_data;

static void w_fun(struct work_struct *w_arg)
{
	struct my_dat *data = container_of(w_arg, struct my_dat, work);
	pr_info("I am in w_fun, jiffies = %ld\n", jiffies);
	pr_info(" I think my current task pid is %d\n", (int)current->pid);
	pr_info(" my data is: %d\n", atomic_read(&data->len));
}

static ssize_t
mycdrv_write(struct file *file, const char __user * buf, size_t lbuf,
	     loff_t * ppos)
{
	struct my_dat *data = (struct my_dat *)&my_data;
	pr_info(" Entering the WRITE function\n");
	pr_info(" my current task pid is %d\n", (int)current->pid);
	pr_info("about to schedule workqueue,  jiffies=%ld\n", jiffies);
	queue_work(my_workq, &data->work);
	pr_info(" i queued the task, jiffies=%ld\n", jiffies);
	atomic_add(100, &data->len);
	return lbuf;
}

static const struct file_operations mycdrv_fops = {
	.owner = THIS_MODULE,
	.read = mycdrv_generic_read,
	.write = mycdrv_write,
	.open = mycdrv_generic_open,
	.release = mycdrv_generic_release,
};

static int __init my_init(void)
{
	struct my_dat *data = (struct my_dat *)&my_data;
	my_workq = create_workqueue("my_workq");
	atomic_set(&data->len, 100);
	INIT_WORK(&data->work, w_fun);
	return my_generic_init();
}

static void __exit my_exit(void)
{
	destroy_workqueue(my_workq);
	my_generic_exit();
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL v2");
