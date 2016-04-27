
#include <linux/module.h>

static DECLARE_WAIT_QUEUE_HEAD (wq);
static atomic_t cond;
static struct task_struct *tsk;

static irqreturn_t my_interrupt (int irq, void *dev_id)
{
    struct my_dat *data = (struct my_dat *)dev_id;
    atomic_inc (&counter_th);
    data->jiffies = jiffies;
    atomic_set (&cond, 1);
    mdelay (delay);             /* hoke up a delay to try to cause pileup */
    wake_up_interruptible (&wq);
    return IRQ_NONE;            /* we return IRQ_NONE because we are just observing */
}

static int thr_fun (void *thr_arg)
{
    struct my_dat *data = (struct my_dat *)thr_arg;

    /* go into a loop and deal with events as they come */

    do {
        atomic_set (&cond, 0);
        wait_event_interruptible (wq, kthread_should_stop ()
                                  || atomic_read (&cond));
        if (atomic_read (&cond))
            atomic_inc (&counter_bh);
        printk
            (KERN_INFO
             "In BH: counter_th = %d, counter_bh = %d, jiffies=%ld, %ld\n",
             atomic_read (&counter_th), atomic_read (&counter_bh),
             data->jiffies, jiffies);
    } while (!kthread_should_stop ());
    return 0;
}

static int __init my_init (void)
{
    atomic_set (&cond, 1);
    if (!(tsk = kthread_run (thr_fun, (void *)&my_data, "thr_fun"))) {
        printk (KERN_INFO "Failed to generate a kernel thread\n");
        return -1;
    }
    return my_generic_init ();
}

static void __exit my_exit (void)
{
    kthread_stop (tsk);
    my_generic_exit ();
}

module_init (my_init);
module_exit (my_exit);
