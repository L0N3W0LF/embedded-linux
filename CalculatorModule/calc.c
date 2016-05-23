/*
 * Operating Systems  (2IN05)  Practical Assignment
 * Kernel modules
 *
 * Ferdinand L. Bermudez (2179624)
 * Torben A. Zurhelle (2193675)
 *
 * Licensed under GLP
 *
 */
#include  <linux/module.h>        /* Specifically, a module */
#include  <linux/kernel.h>        /* We're doing kernel work */
#include  <linux/proc_fs.h>       /* Necessary because we use the proc fs */
#include  <asm/uaccess.h>         /* for copy_from_user */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Torben & Ferdi");
MODULE_DESCRIPTION("EMB2 Kernel Module Math Exercise");

#define CALC_NAME        "calc"

// Structure to keep information about the proc.
static struct proc_dir_entry *Proc_File;

// The buffer for this module.
static long proc_buffer;

/*
 * This function will clear the buffer.
 */
static void clear_buffer(void)
{
    // Start the buffer with '0' as the initial value.
    proc_buffer = 0;
}

/*
 * This function will calculate pow.
 */
static long myPow(long base, long exponent)
{
    long result = 1;
    while (exponent > 0)
    {
        result *= base;
        exponent--;
    }
    return result;
}

/*
 *
 */
static long myCalculate(long operand1, char operator, long operand2)
{
	// Variable to store the result.
	long result = 0;
	
    // Calculate.
    switch (operator)
    {
        case '+': result = operand1 + operand2; break;
        case '-': result = operand1 - operand2; break;
        case '/':
            if (operand2 != 0)
            {
                result = operand1 / operand2;
            }
            else
            {
                printk (KERN_INFO "calc_write: cannot devide by zero\n");
                result = operand1;
            }
            break;
        case '*': result = operand1 * operand2; break;
        case '%':
            if (operand2 != 0)
            {
                result = operand1 % operand2;
            }
            else
            {
                printk (KERN_INFO "calc_write: cannot mod by zero\n");
                result = operand1;
            }
            break;
        case '^': result = myPow(operand1, operand2); break;
        case 'c': result = 0; break;
		default: return operand1;
    }
	// Return the result.
	return result;
}

/*
 * This function is called when the /proc file is read
 */
static int calc_read (char *buffer,
                      char **buffer_location,
                      off_t offset, int buffer_length, int *eof, void *data)
{
    int bytesRead;      // To hold how many bytes have been read from buffer.
	char *temp;

    printk(KERN_INFO "calc_read (/proc/%s) called\n", CALC_NAME);

    if (offset > 0)
    {
        bytesRead = 0;      // 0 means we are not done reading.
    }
    else
    {
		bytesRead = sprintf(temp, "\d\n", proc_buffer);
        copy_to_user(buffer, temp, bytesRead);
    }

    return bytesRead;
}

/*
 * This function is called when the /proc file is written
 */
static int calc_write (struct file *file, const char *buffer, unsigned long count,
                       void *data)
{
    char operator;
    long operand1;
    long operand2;

    printk(KERN_INFO "calc_write (/proc/%s) called\n", CALC_NAME);

    // Get the operator from the user buffer.
    operator = buffer[0];

    // Get the operands from module buffer and user buffer.
    operand1 = proc_buffer;
    operand2 = simple_strtol(buffer+1, NULL, 10);

    // Calculate and store the result to te module buffer.
    proc_buffer = myCalculate(operand1, operator, operand2);

    return count;
}

/*
 * This function is called when the module is loaded
 */
int init_module (void)
{
    Proc_File = create_proc_entry(CALC_NAME, 0666 ,NULL);

    if (Proc_File == NULL)
    {
        remove_proc_entry (CALC_NAME, NULL);
        printk (KERN_INFO "/proc/%s failed to load\n", CALC_NAME);
        return -ENOMEM;
    }

    Proc_File->read_proc = calc_read;
    Proc_File->write_proc = calc_write;
    //Proc_File->mode = S_IFREG | S_IRUGO;
    Proc_File->uid = 0;
    Proc_File->gid = 0;
    Proc_File->size = 80;

    // Clear the buffer.
    clear_buffer();

    // Print the welcome line.
    printk (KERN_INFO "/proc/%s loaded\n", CALC_NAME);

    // Return 0 if everything is ok.
    return 0;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module (void)
{
    /* JG,090727: this is boiler-plate stuff copied from
     * The Linux Kernel Module Programming Guide
     *
     * Apparently, newer kernels do not export 'proc_root' anymore;
     * it is being advised to change '&proc_root' into 'NULL'
     * (see also  http://www.digitalruin.net/node/43 and
     * http://code.google.com/p/eeepc-linux/issues/detail?id=15)
     *
     * Original code:
     *      remove_proc_entry (CALC_NAME, &proc_root);
     *
     * The change seems to work properly for Ubuntu 8.04.1 (the 2IN05 VMware image)
     */
    remove_proc_entry (CALC_NAME, NULL);
    printk (KERN_INFO "/proc/%s removed\n", CALC_NAME);
}