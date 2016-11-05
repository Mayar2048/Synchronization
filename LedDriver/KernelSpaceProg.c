#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <stdbool.h>
#include <linux/ioctl.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "DeviceHeaders.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mayar Abd Elaziz");
MODULE_DESCRIPTION("Synchronization in Linux Kernel");

static int majorNumber;
static char ledsValue[LENGTH] = {'0', '0', '0'};      // {caps, num, scroll}
static int ledsValueSize;
static unsigned long ledNum;
static struct class* ledModuleClass     = NULL;
static struct device* ledModuleDevice   = NULL;


static int dev_open(struct inode*, struct file*);
static int dev_release(struct inode*, struct file*);
static ssize_t dev_read(struct file*, char*, size_t, loff_t*);
static ssize_t dev_write(struct file*, const char*, size_t, loff_t*);
static long dev_ioctl(struct file *, unsigned int, unsigned long);
static void set_led_state(int, int);
static int get_led_state(int);
static unsigned char get_led_status_word(void);
static int update_leds(unsigned char);
static bool get_bit(unsigned char, int);

static struct semaphore sema;

static struct file_operations operations =
{
    .open       =   dev_open,
    .release    =   dev_release,
    .read       =   dev_read,
    .write      =   dev_write,
    .unlocked_ioctl     =   dev_ioctl,
};


/**
*Purpose: init semaphore to prevent race condition
*/
void init(void){
    sema_init(&sema, COUNTER_VALUE);
}


/**
*Purpose: init linux kernel module
*@return flag to determie success or failure
*/
static int __init ledModule_init(void)
{
    init();
    printk(KERN_INFO "PID [%d]:\tInitializing the Linux Kernel Module\n", current->pid);

    majorNumber = register_chrdev(0, DEVICE_NAME, &operations);
    if(majorNumber < 0)
    {
        printk(KERN_ALERT "PID [%d]:\tFailed to register major number for this device!!\n", current->pid);
        return majorNumber;
    }

    ledModuleClass = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(ledModuleClass))
    {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "PID [%d]:\tFailed to register device class!!\n", current->pid);
        return PTR_ERR(ledModuleClass);
    }

    ledModuleDevice = device_create(ledModuleClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if(IS_ERR(ledModuleDevice))
    {
        class_destroy(ledModuleClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "PID [%d]:\tFailed to create the device!!\n", current->pid);
        return PTR_ERR(ledModuleDevice);
    }

    printk(KERN_INFO "PID [%d]:\tLinux Kernel Module is created successfully\n", current->pid);
    return SUCCESS;
}


/**
*Purpose: exit linux kernel module
*/
static void __exit ledModule_exit(void)
{
    device_destroy(ledModuleClass, MKDEV(majorNumber, 0));
    class_unregister(ledModuleClass);
    class_destroy(ledModuleClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "PID [%d]:\tUninstalling Linux Kernel Module is done successfully\n", current->pid);
}


/**
*Purpose: open ledModule device file
*@param pointer to the ledModule file
*@return flag to determie success
*/
static int dev_open(struct inode* inodep, struct file* filep)
{
    printk(KERN_INFO "PID [%d]:\tDevice is created successfully\n", current->pid);
    return SUCCESS;
}


/**
*Purpose: close ledModule device file
*@param pointer to the ledModule file
*@return flag to determie success
*/
static int dev_release(struct inode* inodep, struct file* filep)
{
    printk(KERN_INFO "PID [%d]:\tDevice is closed successfully\n", current->pid);
    return SUCCESS;
}


/**
*Purpose: read led's state from the ledModule
*@param pointer to the ledModule file, buffer to copy data in, length of buffer, memory offset
*@return the specified led state
*/
static ssize_t dev_read(struct file* filep, char* buffer, size_t len, loff_t* offset)
{
    int ledValue = get_led_state(ledNum);
    return ledValue;
}


/**
*Purpose: write led's state to the ledModule
*@param pointer to the ledModule file, buffer to copy data in, length of buffer, memory offset
*@return length of buffer
*/
static ssize_t dev_write(struct file* filep, const char* buffer, size_t len, loff_t* offset)
{
    sprintf(ledsValue, "%s", buffer);
    ledsValueSize = strlen(ledsValue);
    printk(KERN_ALERT "PID [%d]:\tLeds' states are(%s)\n", current->pid, ledsValue);
    return len;
}


/**
*Purpose: control the process of read and write and to determine the led
*@param pointer to the ledModule file, command number, additional param to specify the led
*@return flag to determie success or failure
*/
static long dev_ioctl(struct file *filep, unsigned int ioctl_num, unsigned long ioctl_param)
{
    int controller_return;

    switch(ioctl_num)
    {
    case IOCTL_SET_LED_STATE:;
        // ioctl_param  <led,state>
        int ledIndexState = (int)ioctl_param;

        int led_state = ledIndexState % 10;

        ledIndexState /= 10;
        int led_index = ledIndexState % 10;
        down(&sema);
        set_led_state(led_index, led_state);
        up(&sema);
        controller_return = dev_write(filep, ledsValue, LENGTH, 0);
        if (controller_return < 0)
        {
            printk(KERN_ALERT "PID [%d]:\tFailed to write the message to the device.\n", current->pid);
            return -1;
        }

        unsigned char status_word = get_led_status_word();

        update_leds(status_word);
        break;

    case IOCTL_GET_LED_STATE:
        // ioctl_param  <led>
        ledNum = ioctl_param;
        printk(KERN_INFO "LED NUM = %d\n", ledNum);

        controller_return = dev_read(filep, ledsValue, LENGTH, 0);
        if (controller_return < 0)
        {
            printk(KERN_ALERT "PID [%d]:\tFailed to read the message from the device.\n", current->pid);
            return -1;
        }
        return controller_return;
    }
    return SUCCESS;
}


/**
*Purpose: change led state according to user command
*@param led number and state
*/
static void set_led_state(int led, int state)
{
    ledsValue[2 - led] = state +'0';
}


/**
*Purpose: get the state of a specific led
*@param led number
*@return led state
*/
static int get_led_state(int led)
{
    down(&sema);
    int state = ledsValue[2 - led] - '0';
    up(&sema);
    return state;
}


/**
*Purpose: get the overall leds' states
*@return leds' states
*/
static unsigned char get_led_status_word(void)
{
    unsigned char sum = 0;
    int i;
    int multiplier = 0;
    for(i = 2; i >= 0; i--)
    {
        sum += (int)((ledsValue[i] - 48) * (1 << multiplier++));
    }
    return sum;
}


/**
*Purpose: read the status of PS/2 keyboard
*@return status
*/
static unsigned char kbd_read_status(void)
{
    return inb(0x64);
}


/**
*Purpose: read data from PS/2 keyboard
*@return data
*/
static unsigned char kbd_read_data(void)
{
    unsigned char controller_status;

    do
    {
        controller_status = kbd_read_status();
    }
    while(!get_bit(controller_status, 0));

    return inb(0x60);
}


/**
*Purpose: write data to PS/2 keyboard
*@param data
*/
static void kbd_write_data(unsigned char data)
{
    unsigned char controller_status;

    do
    {
        controller_status = kbd_read_status();
    }
    while(get_bit(controller_status, 1));

    outb(data, 0x60);
}


/**
*Purpose: update leds' states on the PS/2 keyboard
*@param the overall leds' states
*@return flag to determie success or failure
*/
static int update_leds(unsigned char led_status_word)
{
    down(&sema);
    printk(KERN_INFO "PID [%d]:\tSTART\n", current->pid);
    unsigned char ret;
    disable_irq(1);
    kbd_write_data(0xED);
    printk(KERN_INFO "PID [%d]:\tSending 0xED Command\n", current->pid);

    if(kbd_read_data() != 0xFA)
    {
        enable_irq(1);
        up(&sema);
        return -1;
    }
    printk(KERN_INFO "PID [%d]:\tRecieved ACK\n", current->pid);

    printk(KERN_INFO "PID [%d]:\tSleep for 500 ms\n", current->pid);
    msleep(50);
    printk(KERN_INFO "PID [%d]:\tWake Up\n", current->pid);

    kbd_write_data(led_status_word);
    printk(KERN_INFO "PID [%d]:\tSending Keyboard Data\n", current->pid);

    if(kbd_read_data() != 0xFA)
    {
        enable_irq(1);
        up(&sema);
        return -1;
    }
    printk(KERN_INFO "PID [%d]:\tRecieved another ACK\n", current->pid);
    enable_irq(1);
    printk(KERN_INFO "PID [%d]:\tEXIT\n", current->pid);
    up(&sema);
    return SUCCESS;
}


/**
*Purpose: get a specific bit in the binary representation of an unsigned char
*@param unsigned char, bit number
*@return bool to determine 0 or 1
*/
static bool get_bit(unsigned char num, int bit_num)
{
    return (num & (1 << bit_num)) != 0;
}

module_init(ledModule_init);
module_exit(ledModule_exit);
