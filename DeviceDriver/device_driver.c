/*
 * Linux Operating Systems  (2IN05)  Practical Assignment
 * Kernel Modules
 *
 * Ferdinand L. Bermudez (2179624)
 * Torben A. Zurhelle (2193675)
 *
 */

#include  <linux/module.h>        // Specifically, a module.
#include  <linux/kernel.h>        // We're doing kernel work.
#include <linux/slab.h>           // For kmalloc.
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/errno.h>          // For error codes.
#include <linux/types.h>          // For size_t.
#include <linux/proc_fs.h>
#include <linux/fcntl.h>          // O_ACCMODE.
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/uaccess.h>          // For copy_from/to_user.
#include <asm/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ferdi & Torben");
MODULE_DESCRIPTION("EMB2 Device Driver");

// Declaration of operations.
loff_t device_llseek (struct file *fp, loff_t offset, int whence);
ssize_t device_read (struct file *fp, char *buf, size_t count, loff_t *offset);
ssize_t device_write (struct file *fp, const char *buf, size_t count, loff_t *offset);
int device_open(struct inode *inode, struct file *filp);
int device_release(struct inode *inode, struct file *filp);
int device_init (void);
void device_exit (void);
int resize_buffer (int size, int minor);
int check_minor (int minor);
void init_lock (void);

// Structure that declares the usual file access functions.
struct file_operations device_fops = {
    .llseek = device_llseek,
    .read = device_read,
    .write = device_write,
    .open = device_open,
	.release = device_release,
};

// Major number.
int device_major = 1337;
int device_minor1 = 0;
int device_minor2 = 1;

// Buffer to store data.
char *device_buffer[2] = { NULL, NULL };
int buffer_lenght[2] = { 0, 0 };
int buffer_size[2] = { 0, 0 };
const int MAX_BUFFER_SIZE = 1024;
int buffer_offset[2] = { 0, 0 };

// Spinlock.
spinlock_t lock[2];

// Seek to a possition in the buffer.
loff_t device_llseek (struct file *fp, loff_t offset, int whence)
{
    int minor, i, location;
    long off = offset;

    minor = iminor(fp->f_path.dentry->d_inode);
    if ((i = check_minor(minor)) == -1)
    {
        return -1;
    }

    // 0 = SEEK_SET, 1 = SEEK_CUR, 2 = SEEK_END.
    switch (whence)
    {
        case 0: location = offset; break;
        case 1: location = buffer_offset[i] + offset; break;
        case 2: location = buffer_lenght[i] - offset; break;
        default: location = buffer_offset[i]; break;
    }

    // Check if the offset is within bounds.
    if (location > buffer_lenght[i] || location < 0)
    {
        printk(KERN_INFO "could not seek to that position");
        return -1;
    }

    // Set the offset of the buffer.
    buffer_offset[i] = location;
    return 0;
}

// Read from the device buffer.
ssize_t device_read (struct file *fp, char *buf, size_t count, loff_t *offset)
{
    char *toRead;
    int lenght, minor, i;

    minor = iminor(fp->f_path.dentry->d_inode);
    if ((i = check_minor(minor)) == -1)
    {
        return -1;
    }

    // Make sure the device buffer was loaded.
    if (device_buffer[i] == NULL)
    {
        printk(KERN_INFO "device buffer is not loaded.\n");
        return 0;
    }

    // Stop reading if the offset > 0.
    if (*offset > 0)
    {
        return 0;
    }
    else
    {
        toRead = device_buffer[i]+buffer_offset[i];
        lenght = (buffer_lenght[i] - buffer_offset[i]);

        copy_to_user(buf,toRead,lenght);
        *offset = buffer_lenght[i];
        return buffer_lenght[i];
    }
}

// Write to the device buffer.
ssize_t device_write (struct file *fp, const char *buf, size_t count, loff_t *offset)
{
    int i, minor;

    minor = iminor(fp->f_path.dentry->d_inode);
    if ((i = check_minor(minor)) == -1)
    {
        return -1;
    }

    // Make sure the device buffer was loaded.
    if (device_buffer[i] == NULL)
    {
        printk(KERN_INFO "device buffer is not loaded.\n");
        return 0;
    }

    // Lock.
    spin_lock_irq(&lock[i]);

    // Check if there is enought buffer space.
    if (count > MAX_BUFFER_SIZE)
    {
        printk(KERN_INFO "device_write: max buffer size limit reached\n");
        return 0;
    }
    // If the buffer is to small, make it bigger.
    if (count > buffer_size[i])
    {
        resize_buffer(count, minor);
    }
    // Copy the contents of the user buffer to the device buffer.
    copy_from_user(device_buffer[i],buf,count);
    buffer_lenght[i] = count;

    //Unlock.
    spin_unlock_irq(&lock[i]);

    return count;
}

int device_open(struct inode *inode, struct file *filp)
{
 /* Success */
 return 0;
}

int device_release(struct inode *inode, struct file *filp)
{
 /* Success */
 return 0;
}

// Load the device driver.
int device_init (void)
{
    int i, size = 1;

    // Register the device driver.
    int result = register_chrdev(device_major, "device", &device_fops);
    if (result < 0)
    {
        printk (KERN_INFO "could not claim major number %i\n", device_major);
        return result;
    }

    for (i = 0; i < 2; i++)
    {
        // Allocate the memory for the device buffer.
        device_buffer[i] = kmalloc(size, GFP_KERNEL);
        if (!device_buffer[i])
        {
            printk (KERN_INFO "failed to allocate buffer\n");
            device_exit();
            return -ENOMEM;
        }
        // Set buffer size.
        buffer_size[i] = size;
        // Fill the memory with 0's.
        memset(device_buffer[i], 0, size);
    }

    printk(KERN_INFO "device driver loaded\n");
    return 0;
}

// Remove the device driver.
void device_exit (void)
{
    int i;

    // Unregister the device driver.
    unregister_chrdev(device_major, "device");

    // Free the memory.
    for (i = 0; i < 2; i++)
    {
        if (device_buffer[i])
        {
            kfree(device_buffer[i]);
        }
    }
    printk(KERN_INFO "device driver removed\n");
}

// Resize the current buffer to a new size.
int resize_buffer(int size, int minor)
{
    int i;
    char* temp;

    if ((i = check_minor(minor)) == -1)
    {
        printk(KERN_INFO "invalid minor number\n");
        return -1;
    }
    temp = kmalloc(buffer_size[i], GFP_KERNEL);

    // Make a copy the buffer.
    sprintf(temp, "%s", device_buffer[i]);

    // Re-allocate the buffer to a new size.
    kfree(device_buffer[i]);
    device_buffer[i] = kmalloc(size, GFP_KERNEL);

    // Copy the contents of the old buffer in the new one.
    sprintf(device_buffer[i], "%s", temp);
    buffer_size[i] = size;
    kfree(temp);

    return 0;
}

// Check if the minor is valid.
int check_minor(int minor)
{
    if (minor == device_minor1 || minor == device_minor2)
    {
        return minor;
    }
    else
    {
        printk(KERN_INFO "invalid minor number %i\n", minor);
        return -1;
    }
}

// Initialize the locks.
void init_lock (void)
{
    int i;
    for (i = 0; i < 2; i++)
    {
        lock[i] = __SPIN_LOCK_UNLOCKED();
    }
}

// This function is called when the module is loaded.
int init_module (void)
{
    device_init();
    printk (KERN_INFO "device module loaded\n");
    return 0;
}

// This function is called when the module is unloaded.
void cleanup_module (void)
{
    device_exit();
    printk (KERN_INFO "device module removed\n");
}
