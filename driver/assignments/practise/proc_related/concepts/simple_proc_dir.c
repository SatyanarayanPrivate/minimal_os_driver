#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/list.h>
#include<linux/proc_fs.h>
#include<linux/slab.h>
#include<linux/seq_file.h>

#define DATA_BUFFER_SIZE 20
static struct proc_dir_entry *new_parent = NULL;


struct mydata_struct {
    
    struct list_head list;
    char info_data[DATA_BUFFER_SIZE];    
};

static LIST_HEAD (mylist);

static void *my_seqfile_start (struct seq_file *seq, loff_t *position)
{
    struct mydata_struct *traverse_list;
    loff_t traverse_offset = 0x00;
    
    list_for_each_entry (traverse_list, &mylist, list) {
        if (*position == traverse_offset++) {
            printk ("my_seqfile_start:: started\n");
            return traverse_list;
            
        }
    } 
    printk ("my_seqfile_start:: over\n");
    return NULL;
}

static void* my_seqfile_next (struct seq_file *seq, void *data_iterator, loff_t *position)
{
    
    struct list_head *next_iterator = ((struct mydata_struct *) data_iterator)->list.next;
    
    printk ("my_seqfile_next:: next iterator accessed\n");
    
    ++*position;
    
    return (next_iterator != &mylist) ? list_entry ( next_iterator, struct mydata_struct, list) : NULL;
}

static int my_seqfile_show (struct seq_file *seq, void *data_iterator) {
    int return_val = 0x00;
    struct mydata_struct *mydata_entry = (struct mydata_struct *) data_iterator;
    
    printk ("my_seqfile_show:: showing data to user\n");
    
    return_val = seq_printf (seq,"%s\n", mydata_entry->info_data);
    printk ("my_seqfile_show:: number of bytes read: %d\n", return_val);
    
    return return_val;
    
}

static void my_seqfile_stop (struct seq_file *seq, void *v) {
    printk ("my_seqfile_stop:: stopped\n");
}

static struct seq_operations my_seq_ops = {
   .start = my_seqfile_start,
   .next   = my_seqfile_next,
   .stop   = my_seqfile_stop,
   .show   = my_seqfile_show,
};

static int my_seqfile_open (struct inode *inode, struct file *file) {
    
    printk ("my_seqfile_open:: file opened\n");
    
    
    return seq_open (file, &my_seq_ops);
}


static struct file_operations my_proc_fops = {
   .owner = THIS_MODULE,
   .open  = my_seqfile_open,
    .read = seq_read,
   .llseek = seq_lseek,
   .release = seq_release,
};


// refer "/linux/proc_fs.h" for more practise/ api's related to proc operations

static int __init proc_entry_init (void) {
   
    struct mydata_struct *my_new_entry = NULL;
    
    int data_entry_counter = 0x00;
#define MAX_DATA_ENTRIES 20
    
    new_parent = proc_mkdir ("sample_proc_test", NULL);
    proc_create ("sample_proc", 0, new_parent, &my_proc_fops);
    
    for (data_entry_counter = 0x00; data_entry_counter < MAX_DATA_ENTRIES; data_entry_counter ++) {
        my_new_entry = kmalloc (sizeof(struct mydata_struct), GFP_KERNEL);
        if (my_new_entry != NULL) {
            sprintf(my_new_entry->info_data, "Node No: %d\n", data_entry_counter);
            list_add_tail (&my_new_entry->list, &mylist); 
        }
        else {
            return -1;
        }
    }
    
    
    printk ("%s:: init completed\n", __func__);
    return 0;
}

static void proc_entry_exit (void) { 
    
    //incomplete
    struct mydata_struct *p,*n;
    list_for_each_entry_safe (p,n, &mylist, list) 
        kfree(p);

    remove_proc_entry ("sample_proc", new_parent);
    proc_remove (new_parent);
   
    printk ("%s:: module removed\n", __func__);
    
}

module_init (proc_entry_init);
module_exit (proc_entry_exit);

