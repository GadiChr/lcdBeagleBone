
#include "classAttrRoutines.h"
#include "lcdroutines.h"

static ssize_t command_read(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t command_write(struct class *cls, struct class_attribute *attr,const char *buf, size_t count);

static ssize_t version_read(struct class *cls, struct class_attribute *attr, char *buf);

// "static CLASS_ATTR" will be resolved to "static struct class_attribute class_attr_##_name"
static CLASS_ATTR(version, S_IRUSR , version_read, NULL);
static CLASS_ATTR(command, S_IRUSR|S_IWUSR, command_read, command_write);

int  lcdClassAttr_init(struct class *cls){
  int ret = 0;

  ret = class_create_file(cls, &class_attr_command);
  if(ret) goto lcd_i_exit;

  ret = class_create_file(cls,&class_attr_version);
  if(ret) goto lcd_i_exit;
  
  return ret;

 lcd_i_exit:
  printk(KERN_ALERT "Lcd: cannot create attribute file in /sys/class/");
  return ret;
}

void lcdClassAttr_destroy(void){

}

static ssize_t command_read(struct class *cls, struct class_attribute *attr, char *buf){

  return 0;
}

static ssize_t command_write(struct class *cls, struct class_attribute *attr, const char *buf, size_t count){

  return count;
}

static ssize_t version_read(struct class *cls, struct class_attribute *attr, char *buf){

  return 0;
}

