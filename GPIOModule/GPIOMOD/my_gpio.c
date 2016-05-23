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
#include  <linux/proc_fs.h>       // Necessary because we use the procfs.
#include  <asm/uaccess.h>         // For copy_from_user snd copy_to_user.
#include  <asm/io.h>              // For using pins.

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ferdi & Torben");
MODULE_DESCRIPTION("EMB2 Soerkis GPIO Controller");

#define PROCFS_NAME        "my_gpio"

#define INDEX_REG          0x2e
#define DATA_REG           0x2f

#define BUTTON_PORT        0x09
#define BUTTON1_PIN        0x02
#define BUTTON2_PIN        0x08
#define BUTTON3_PIN        0x20

#define LED1_PORT          0x08
#define LED2_PORT          0x00
#define LED3_PORT          0x04
#define LED1_PIN           0x80
#define LED2_PIN           0x10
#define LED3_PIN           0x04

// Structure to keep information about the proc.
static struct proc_dir_entry *Proc_File;

// Used to set which GPIO to read on read, default case is n (null).
static unsigned char buttonToRead = 'n';

// Generic register write function.
static void write_reg (short value, short reg)
{
    outb_p(value, reg);
}

// Generic register read function.
static short read_reg (short reg)
{
    return inb_p(reg);
}

// Configure the soekris for GPIO use.
static void init_gpio (void)
{
    const unsigned char LDN_REG = 0x07;
    const unsigned char GPIO_MODE = 0x07;

    const unsigned char GPIO_SELECT_REG = 0xf0;
    const unsigned char GPIO_CONF_REG = 0xf1;
    const unsigned char GPIO_READ = 0x01;
    const unsigned char GPIO_WRITE = 0x05;

    const unsigned char btnAddr[3] = {0x21, 0x23, 0x25};
    const unsigned char ledAddr[3] = {0x27, 0x04, 0x12};
    int index;

    // Set the mode to GPIO.
    outb_p(LDN_REG, INDEX_REG);
    outb_p(GPIO_MODE, DATA_REG);

    // Configure the button GPIOs for input.
    for (index = 0; index < 3; index++)
    {
        write_reg(GPIO_SELECT_REG, INDEX_REG);
        write_reg(btnAddr[index], DATA_REG);
        write_reg(GPIO_CONF_REG, INDEX_REG);
        write_reg(GPIO_READ, DATA_REG);
    }

    // Configue the LED GPIOs for output.
    for (index = 0; index < 3; index++)
    {
        write_reg(GPIO_SELECT_REG, INDEX_REG);
        write_reg(ledAddr[index], DATA_REG);
        write_reg(GPIO_CONF_REG, INDEX_REG);
        write_reg(GPIO_WRITE, DATA_REG);
    }
}

// Get the SID from the Soekris device.
static unsigned char read_sid (void)
{
    const unsigned char SID_REG = 0x20;
    const unsigned char SID = 0xe9;

    write_reg(SID_REG, INDEX_REG);
    if (read_reg(DATA_REG) == SID)
    {
        return 1;
    }
    return 0;
}

// Get the base address.
static short get_baseaddr (void)
{
    const unsigned char BASEADDR_LSB = 0x61;
    const unsigned char BASEADDR_MSB = 0x60;
    short baseAddr = 0;

    outb_p(BASEADDR_MSB, INDEX_REG);
    baseAddr = inb_p(DATA_REG);
    baseAddr = (baseAddr << 8);

    outb_p(BASEADDR_LSB, INDEX_REG);
    baseAddr |= inb_p(DATA_REG);

    return baseAddr;
}

// Get the value from buttons.
static unsigned char read_button(unsigned char btnNr)
{
    unsigned char state = 0;
    unsigned char button_pin = 0;
    short baseAddr = get_baseaddr();
    short offsetAddr = baseAddr | BUTTON_PORT;

    // Select the correct pin.
    switch (btnNr)
    {
        case '1':
            button_pin = BUTTON1_PIN;
            break;
        case '2':
            button_pin = BUTTON2_PIN;
            break;
        case '3':
            button_pin = BUTTON3_PIN;
            break;
        default:
            return 'n';
            break;
    }

    // Read the pin for the button.
    if ((read_reg(offsetAddr) & button_pin) == 0)
    {
        state = '1';
    }
    else
    {
        state = '0';
    }

    return state;
}

// Change the value of LEDS.
static void write_led(unsigned char ledNr, unsigned char ledVal)
{
    unsigned char currentState = 0;
    unsigned char led_port = 0;
    unsigned char led_pin = 0;
    short offsetAddr = 0;
    short baseAddr = get_baseaddr();

    // Select the correct port and pin.
    switch (ledNr)
    {
        case '1':
            led_port = LED1_PORT;
            led_pin = LED1_PIN;
            break;
        case '2':
            led_port = LED2_PORT;
            led_pin = LED2_PIN;
            break;
        case '3':
            led_port = LED3_PORT;
            led_pin = LED3_PIN;
            break;
        default:
            return;
            break;
    }

    // Get the baseaddress+offset.
    offsetAddr = baseAddr | led_port;

    // Get the current bits and toggle the bit for the led.
    currentState = read_reg(offsetAddr);
    if (ledVal == '0')
    {
        write_reg(currentState & (~led_pin), offsetAddr);
    }
    else
    {
        write_reg(currentState | led_pin, offsetAddr);
    }
}

// This function is called when the /proc file is read.
static int gpio_read (char *buffer, char **buffer_location, off_t offset,
                      int buffer_length, int *eof, void *data)
{
    short bytesRead = 0;
    unsigned char state = 'n';      // Default case is n (null).

    if (offset > 0)
    {
        bytesRead = 0;              // 0 means we are not done reading.
    }
    else
    {
        // Get the state of the button that is defined in buttonToRead.
        if (buttonToRead != 'n')
        {
            state = read_button(buttonToRead);
        }
        else
        {
            printk (KERN_INFO "/proc/%s gpio_read error\n", PROCFS_NAME);
        }
        bytesRead = sprintf(buffer, "%c\n", state);
    }
    return bytesRead;
}

// This function is called when the /proc file is written.
static int gpio_write (struct file *file, const char *buffer,
                       unsigned long count, void *data)
{
    unsigned char option = 0;
    unsigned char id = 0;
    unsigned char value = 0;

    sscanf(buffer, "%c%c%c", &option, &id, &value);

    switch (option)
    {
        case 'l':
            write_led(id, value);
            break;
        case 'b':
            buttonToRead = id;
            break;
        default:
            printk (KERN_INFO "/proc/%s gpio_write error\n", PROCFS_NAME);
            break;
    }
    return count;
}

// This function is called when the module is loaded.
int init_module (void)
{
    Proc_File = create_proc_entry(PROCFS_NAME, 0666 ,NULL);

    if (Proc_File == NULL)
    {
        remove_proc_entry (PROCFS_NAME, NULL);
        printk (KERN_INFO "/proc/%s failed to load\n", PROCFS_NAME);
        return -ENOMEM;
    }

    Proc_File->read_proc = gpio_read;
    Proc_File->write_proc = gpio_write;
    Proc_File->mode |= S_IFREG | S_IRUGO;
    Proc_File->uid = 0;
    Proc_File->gid = 0;
    Proc_File->size = 80;

    // Check if the SID is correct.
    if (read_sid() == 0)
    {
        printk (KERN_INFO "/proc/%s wrong sid\n", PROCFS_NAME);
        return -1;
    }

    // Configure the Soekris for GPIO use.
    init_gpio();

    printk (KERN_INFO "/proc/%s loaded\n", PROCFS_NAME);
    return 0;
}

// This function is called when the module is unloaded.
void cleanup_module (void)
{
    remove_proc_entry (PROCFS_NAME, NULL);
    printk (KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}