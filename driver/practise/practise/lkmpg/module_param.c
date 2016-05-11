#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>

static int mymodule_int = 0x01;
static short mymodule_short = 0x05;
static long mymodule_lont_int = 0x999;
static char *mymodule_string = "string_data";
static int mymodule_array [10] = {-3, -4};
static int arr_argc = 0x00;

/*
* module_param(foo, int, 0000)
* The first param is the parameters name
* The second param is it's data type
* The final argument is the permissions bits,
* for exposing parameters in sysfs (if non−zero) at a later stage.
*/
module_param (mymodule_int, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC (mymodule_int, "integer value");

module_param (mymodule_short, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC (mymodule_short, "short value");

module_param (mymodule_lont_int, long, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC (mymodule_lont_int, "long int value");

module_param (mymodule_string, charp, 0000);
MODULE_PARM_DESC (mymodule_string, "character string value");
    
/*
* module_param_array(name, type, num, perm);
* The first param is the parameter's (in this case the array's) name
* The second param is the data type of the elements of the array
* The third argument is a pointer to the variable that will store the number
* of elements of the array initialized by the user at module loading time
* The fourth argument is the permission bits
*/

module_param_array (mymodule_array, int, &arr_argc, 0000);
MODULE_PARM_DESC (mymodule_array, "An array of integers");

static int __init mymodule_init (void) {
    
    int counter = 0x00;
    int array_size = 0x00;
    
    
    printk (KERN_INFO "\nSample Module param !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    
    printk (KERN_INFO "\nmymodule_int: %d", mymodule_int);
    printk (KERN_INFO "\nmymodule_lont_int: %ld", mymodule_lont_int);
    printk (KERN_INFO "\nmymodule_short: %hd", mymodule_short);
    printk (KERN_INFO "\nmymodule_string: %s:", mymodule_string);
    
    array_size = sizeof (mymodule_array)/(sizeof(mymodule_array[0]));
    
    printk (KERN_INFO "\nmymodule_array elements: ");
    for (counter = 0x00; counter < array_size; counter ++) {
        printk (KERN_INFO "\nmymodule_array[%d]: %d", counter, mymodule_array[counter]);
    }
    printk(KERN_INFO "\ngot %d arguments for myintArray.\n", arr_argc);
    return 0;
}

static void __exit mymodule_exit(void) {
    printk(KERN_INFO "Goodbye, sample module param\n");
}
module_init (mymodule_init);
module_exit (mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sample auther");
MODULE_DESCRIPTION ("Sample module parameter");



/* Execute this in following manner by checking syslogs
 * 
 * $ sudo insmod module_param.ko mymodule_int=444
 * $ sudo insmod module_param.ko mymodule_array=444
 */

