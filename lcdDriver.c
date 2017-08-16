/**
 * @file lcdDriver.c  
 * @author Christoph Gadinger
 * @date xx.08.2017
 * @version 
 * @brief A kernel module to drive a character lcd display 
 * which is supposed to be connected via a levelshift circuit (3.3V to 5V) to the gpios 
 * from a BeagleBoneBlack. The driver is compartible with the HD44780 LCD controller.
 * Many parts of this code are derived from Derek Molloys' code examples 
 * (https://github.com/derekmolloy/exploringBB.git)
 */


#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <asm/uaccess.h>          // Required for the copy to user function
#include <linux/mutex.h>          // Required for the mutex functional
#include <linux/string.h>
#include "lcdroutines.h"


#define  DEVICE_NAME "lcdchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "lcdchar"    ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");                  ///< The license type -- this affects available functionality
MODULE_AUTHOR("Christoph Gadinger");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("lcd Module");       ///< The description -- see modinfo
MODULE_VERSION("17.08.14");             ///< A version number to inform users


static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message_passed[100] = {0};    ///< Memory for the string that is passed from userspace
static short  size_of_message_passed;       ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  lcdClass  = NULL;     ///< The device-driver class struct pointer
static struct device* lcdDevice = NULL;     ///< The device-driver device struct pointer

static DEFINE_MUTEX(lcd_mutex);   // Macro to declare a new mutex

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
  {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
  };

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init lcddrv_init(void){
  printk(KERN_INFO "Lcd: Initializing the Lcd LKM\n");

  // 1.    : fourbitmode
  // 2.    : rs_pinNr
  // 3.    : rw_pinNr (set to 255 for allways read, i.e. wire is connected to ground)
  // 4.    : enable_pinNr
  // [5-12]: data_pinNr[0-7]
  //  lcd_init(true, 66, 67, 69, 68, 45, 44, 26, 47, 46, 27, 65);
  lcd_init(false, 66, 67, 69, 68, 45, 44, 26, 47, 46, 27, 65);
  
  printk(KERN_INFO "Lcd: lcd initialization complete\n");
  strcpy(message_passed, "  *     LCD     *  \n  * initialized *");

  lcd_update(message_passed);
  lcd_cursor();
  lcd_blink();
  
  // Try to dynamically allocate a major number for the device -- more difficult but worth it
  majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
  if (majorNumber<0){
    printk(KERN_ALERT "Lcd: failed to register a major number\n");
    return majorNumber;
  }
  printk(KERN_INFO "Lcd: registered correctly with major number %d\n", majorNumber);

  // Register the device class
  lcdClass = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(lcdClass)){                // Check for error and clean up if there is
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "Lcd: Failed to register device class\n");
    return PTR_ERR(lcdClass);          // Correct way to return an error on a pointer
  }
  printk(KERN_INFO "Lcd: device class registered correctly\n");

  // Register the device driver
  lcdDevice = device_create(lcdClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
  if (IS_ERR(lcdDevice)){               // Clean up if there is an error
    class_destroy(lcdClass);           // Repeated code but the alternative is goto statements
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "Lcd: Failed to create the device\n");
    return PTR_ERR(lcdDevice);
  }
  printk(KERN_INFO "Lcd: device class created correctly\n"); // Made it! device was initialized
  mutex_init(&lcd_mutex);       // Initialize the mutex dynamically
  return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit lcddrv_exit(void){
  lcd_uninit();
  
  mutex_destroy(&lcd_mutex);                           // destroy the dynamically-allocated mutex
  device_destroy(lcdClass, MKDEV(majorNumber, 0));     // remove the device
  class_unregister(lcdClass);                          // unregister the device class
  class_destroy(lcdClass);                             // remove the device class
  unregister_chrdev(majorNumber, DEVICE_NAME);         // unregister the major number
  printk(KERN_INFO "Lcd: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){
  if(!mutex_trylock(&lcd_mutex)){
    printk(KERN_ALERT "Lcd: Device in use by another process");
    return -EBUSY;
  }
  numberOpens++;
  printk(KERN_INFO "Lcd: Device has been opened %d time(s)\n", numberOpens);
  return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
  int error_count = 0;
  // copy_to_user has the format ( * to, *from, size) and returns 0 on success
  error_count = copy_to_user(buffer, message_passed, size_of_message_passed);

  if (error_count==0){            // if true then have success
    printk(KERN_INFO "Lcd: Sent %d characters to the user\n", size_of_message_passed);
    ///return (size_of_message_passed=0);  // clear the position to the start and return 0
    return 0;
  }
  else {
    printk(KERN_INFO "Lcd: Failed to send %d characters to the user\n", error_count);
    return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
  }
}

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
  int error_count;

  error_count = copy_from_user(message_passed, buffer, len);
  if(error_count){
    printk(KERN_ALERT "Lcd: Could not receiver %d characters", error_count);
  }

  size_of_message_passed = strlen(message_passed);                 // store the length of the stored message
  printk(KERN_INFO "Lcd: Received %zu characters from the user\n", len);

  lcd_updaten(message_passed, len);
  
  return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
  mutex_unlock(&lcd_mutex);           // release the mutex (i.e., lock goes up)
  printk(KERN_INFO "Lcd: Device successfully closed\n");
  return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(lcddrv_init);
module_exit(lcddrv_exit);
