#ifndef _CLASSATTRROUTINES_H
#define _CLASSATTRROUTINES_H

#include <linux/device.h>
#include <linux/kernel.h>

int  lcdClassAttr_init(struct class *cls);
void lcdClassAttr_destroy(void);

#endif
