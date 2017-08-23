#ifndef _DEVROUTINES_H
#define _DEVROUTINES_H

#include "lcdroutines.h"
#include "classAttrRoutines.h"

#include <linux/mutex.h>          // Required for the mutex functional
#include <asm/uaccess.h>          // Required for the copy to user functino
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/fs.h>             // Header for the Linux file system support


#define  DEVICE_NAME "lcdchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "lcdchar"    ///< The device class -- this is a character device driver


int dev_init(void);
int dev_destroy(void);

#endif
