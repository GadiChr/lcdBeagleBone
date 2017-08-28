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

#include "devroutines.h"
#include "lcdroutines.h"

#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/string.h>

MODULE_LICENSE("GPL");                  ///< The license type -- this affects available functionality
MODULE_AUTHOR("Christoph Gadinger");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("lcd Module");       ///< The description -- see modinfo
MODULE_VERSION("17.08.14");             ///< A version number to inform users


/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init lcddrv_init(void){
  int retVal = 0;

  retVal = dev_init();
  if(retVal) {
    return retVal;
  }
  
  // 1.    : fourbitmode
  // 2.    : rs_pinNr
  // 3.    : rw_pinNr (set to 255 for allways read, i.e. wire is connected to ground)
  // 4.    : enable_pinNr
  // [5-12]: data_pinNr[0-7]
  //  lcd_init(true, 66, 67, 69, 68, 45, 44, 26, 47, 46, 27, 65);
  lcd_init(20, 2, false, 66, 67, 69, 68, 45, 44, 26, 47, 46, 27, 65);
  
  lcd_cursor();
  //  lcd_blink();
  //  lcd_rightToLeft();
  //  lcd_autoscroll();
  //  lcd_scrollDisplayLeft();
  lcd_update("  *     LCD     *  \n  * initialized *");
  
  printk(KERN_INFO "Lcd: lcd initialization complete\n");
  
  return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit lcddrv_exit(void){
  lcd_uninit();

  dev_destroy();
  
  printk(KERN_INFO "Lcd: Goodbye from the LKM!\n");
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(lcddrv_init);
module_exit(lcddrv_exit);
