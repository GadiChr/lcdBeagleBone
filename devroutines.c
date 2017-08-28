
#include "devroutines.h"

static int    majorNumber;                  // Stores the device number -- determined automatically
static char   message_passed[100] = {0};    // Memory for the string that is passed from userspace
static size_t size_of_message_passed;       // Used to remember the size of the string stored
static int    numberOpens = 0;              // Counts the number of times the device is opened
static struct class*  lcdClass  = NULL;     // The device-driver class struct pointer
static struct device* lcdDevice = NULL;     // The device-driver device struct pointer

// Macro to declare a new mutex
static DEFINE_MUTEX(lcd_mutex);

// The prototype functions for the character driver
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

 
// Device is represented as file structure in the kernel
static struct file_operations fops =
  {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
  };

/** 
 *  Funktion to initialize the module's device class
 */ 
int dev_init(){
  int ret = 0;
  
  // Try to dynamically allocate a major number for the device
  majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
  if (majorNumber<0){
    printk(KERN_ALERT "Lcd: failed to register a major number\n");
    return majorNumber;
  }
  printk(KERN_INFO "Lcd: device registered correctly with major number %d\n", majorNumber);

  // Register the device class
  lcdClass = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(lcdClass)){
    printk(KERN_ALERT "Lcd: Failed to register device class\n");
    ret =  PTR_ERR(lcdClass);
    goto dev_init_exit2;
  }
  printk(KERN_INFO "Lcd: device class registered correctly\n");
  
  // Register the device driver
  lcdDevice = device_create(lcdClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
  if (IS_ERR(lcdDevice)){
    printk(KERN_ALERT "Lcd: Failed to create the device\n");
    ret =  PTR_ERR(lcdDevice);
    goto dev_init_exit1;
  }
  printk(KERN_INFO "Lcd: device class created correctly\n"); 

  // add class attributes
  ret = lcdClassAttr_init(lcdClass);
  if(ret) goto dev_init_exit1;
  
  mutex_init(&lcd_mutex);
  return ret;

 dev_init_exit1:
  class_destroy(lcdClass);
  
 dev_init_exit2:
  unregister_chrdev(majorNumber, DEVICE_NAME);

  return ret;
}

/** 
 *  Function to cleanup the module's device class
 */
int dev_destroy(){
  mutex_destroy(&lcd_mutex);
  lcdClassAttr_destroy();
  device_destroy(lcdClass, MKDEV(majorNumber, 0));
  class_unregister(lcdClass);
  class_destroy(lcdClass);
  unregister_chrdev(majorNumber, DEVICE_NAME);
  return 0;
}

/** 
 *  The device open function that is called each time the device is opened
 */
static int dev_open(struct inode *inodep, struct file *filep){
  if(!mutex_trylock(&lcd_mutex)){
    printk(KERN_ALERT "Lcd: Device in use by another process");
    return -EBUSY;
  }
  numberOpens++;
  
  return 0;
}

/** 
 *  This function is called whenever device is being read from user space
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t to_copy, loff_t *offset){
  unsigned long not_copied;

  to_copy = min(to_copy, size_of_message_passed);
  if((not_copied = copy_to_user(buffer, message_passed, to_copy))){
    printk(KERN_INFO "Lcd: Failed to send %lu characters\n", not_copied);
  }
  
  printk(KERN_INFO "Lcd: Sent %d characters to the user\n", to_copy);
  *offset += to_copy - not_copied;

  return to_copy - not_copied;
}

/** 
 *  This function is called whenever the character device is being written to from user space 
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

/** 
 *  The device release function that is called whenever the device is closed/released by
 *  the userspace program
 */
static int dev_release(struct inode *inodep, struct file *filep){
  mutex_unlock(&lcd_mutex);           // release the mutex (i.e., lock goes up)
  printk(KERN_INFO "Lcd: Device successfully closed\n");
  return 0;
}
