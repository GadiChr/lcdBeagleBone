#ifndef _CLASSATTRROUTINES_H
#define _CLASSATTRROUTINES_H

#include "lcdroutines.h"

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>   // for kmalloc

int  lcdClassAttr_init(struct class *cls);
void lcdClassAttr_destroy(void);

#endif
