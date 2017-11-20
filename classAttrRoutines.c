
#include "classAttrRoutines.h"

static ssize_t display_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t display_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count);

static ssize_t blink_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t blink_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count);

static ssize_t cursor_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t cursor_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count);

static ssize_t position_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t position_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count);

static ssize_t autoscroll_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t autoscroll_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count);

static ssize_t textflow_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t textflow_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count);

static ssize_t scroll_show(struct class *cls, struct class_attribute *attr, char *buf);
static ssize_t scroll_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count);

// helper functions
static ssize_t show_on_off(bool isOn, char *buf);
static ssize_t exec_on_off(void (*exec_on)(void), void (*exec_off)(void), const char *buf, size_t count);
static ssize_t show_right_left(bool isRight, char *buf);
static ssize_t exec_right_left(void (*exec_right)(void), void (*exec_left)(void), const char *buf, size_t count);
  
// "static CLASS_ATTR" will be resolved to "static struct class_attribute class_attr_<name>"
static CLASS_ATTR(display,    S_IRUGO|S_IWUSR, display_show,    display_store);
static CLASS_ATTR(blink,      S_IRUGO|S_IWUSR, blink_show,      blink_store);
static CLASS_ATTR(cursor,     S_IRUGO|S_IWUSR, cursor_show,     cursor_store);
static CLASS_ATTR(position,   S_IRUGO|S_IWUSR, position_show,   position_store);
static CLASS_ATTR(autoscroll, S_IRUGO|S_IWUSR, autoscroll_show, autoscroll_store);
static CLASS_ATTR(textflow,   S_IRUGO|S_IWUSR, textflow_show,   textflow_store);
static CLASS_ATTR(scroll,     S_IRUGO|S_IWUSR, scroll_show,     scroll_store);

/** 
 *  Initialize sysfs class attributes
 */
int  lcdClassAttr_init(struct class *cls){
  int ret = 0;

  ret = class_create_file(cls, &class_attr_display);
  if(ret) goto lcd_i_exit;

  ret = class_create_file(cls, &class_attr_blink);
  if(ret) goto lcd_i_exit;

  ret = class_create_file(cls, &class_attr_cursor);
  if(ret) goto lcd_i_exit;

  ret = class_create_file(cls, &class_attr_position);
  if(ret) goto lcd_i_exit;

  ret = class_create_file(cls, &class_attr_autoscroll);
  if(ret) goto lcd_i_exit;

  ret = class_create_file(cls, &class_attr_textflow);
  if(ret) goto lcd_i_exit;
  
  ret = class_create_file(cls, &class_attr_scroll);
  if(ret) goto lcd_i_exit;
  
  return ret;

 lcd_i_exit:
  printk(KERN_ALERT "Lcd: cannot create attribute file in /sys/class/");
  return ret;
}

void lcdClassAttr_destroy(void){

}

// ****** DISPLAY ON/OFF ******
static ssize_t display_show(struct class *cls, struct class_attribute *attr, char *buf){
  return show_on_off(lcd_isDisplayOn(), buf);
}
static ssize_t display_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count){
  return exec_on_off(lcd_display, lcd_noDisplay, buf, count);
}

// ****** BLINK CURSOR ON/OFF ******
static ssize_t blink_show(struct class *cls, struct class_attribute *attr, char *buf){
  return show_on_off(lcd_isBlinkOn(), buf);
}
static ssize_t blink_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count){
  return exec_on_off(lcd_blink, lcd_noBlink, buf, count);
}

// ****** SHOW CURSOR ON/OFF ******
static ssize_t cursor_show(struct class *cls, struct class_attribute *attr, char *buf){
  return show_on_off(lcd_isCursorOn(), buf);
}
static ssize_t cursor_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count){
  return exec_on_off(lcd_cursor, lcd_noCursor, buf, count);
}


// ****** SET CURSOR TO POSITION n:n ******
static ssize_t position_show(struct class *cls, struct class_attribute *attr, char *buf){
  sprintf(buf, "%d:%d\n", lcd_getCursorPosCol(), lcd_getCursorPosRow());
  return strlen(buf) + 1;
}
static ssize_t position_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count){
  static const char DELIMITERS[] = " \n\r:;,.";

  char *string, *tok, *found;
  u8 col, row;

  // allocate space for string
  string = kmalloc(count + 1, GFP_USER);  // GFP_USER from linux/mm.h
  if(string == NULL) return count;  

  string[count] = '\0';
  
  printk(KERN_INFO "LCD: count = %d\n", count);
  
  // copy buf into string
  strncpy(string, buf, count);
  tok = string;

  printk(KERN_INFO "LCD: tok = %s\n", tok);

  // get first value
  found = strsep(&tok, DELIMITERS);
  printk(KERN_INFO "LCD: found1 = %s\n", found);
  
  // convert parsed string to unsigned char
  if(kstrtou8(found, 10, &col));      // suppress compiler warnig by checking returnvalue
                                      // on Success: return -> 0;
                                      // on Error: return -> -EINVAL; col -> 0;
  printk(KERN_INFO "LCD: col = %d\n", col);
  
  found = strsep(&tok, DELIMITERS);
  printk(KERN_INFO "LCD: found2 = %s\n", found);
  
  if(kstrtou8(found, 10, &row));
  printk(KERN_INFO "LCD: row = %d\n", row);

  
  
  // set cursor to desired position 
  lcd_setCursor(col, row);

  kfree(string);
  
  return count;
}

// ****** AUTOSCROLL DISPLAY ENTRY ON/OFF ******
static ssize_t autoscroll_show(struct class *cls, struct class_attribute *attr, char *buf){
  return show_on_off(lcd_isAutoscroll(), buf);
}
static ssize_t autoscroll_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count){
  return exec_on_off(lcd_autoscroll, lcd_noAutoscroll, buf, count);
}


// ****** SCROLL DISPLAY ******
static ssize_t scroll_show(struct class *cls, struct class_attribute *attr, char *buf){
  strcpy(buf, "left/right\n");
  return strlen(buf) + 1;
}
static ssize_t scroll_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count){
  return exec_right_left(lcd_scrollDisplayRight, lcd_scrollDisplayLeft, buf, count);
}

// ****** TEXTFLOW ******
static ssize_t textflow_show(struct class *cls, struct class_attribute *attr, char *buf){
  return show_right_left(lcd_isLeftToRight(), buf);
}
static ssize_t textflow_store(struct class *cls, struct class_attribute *attr,const char *buf, size_t count){
  return exec_right_left(lcd_leftToRight, lcd_rightToLeft, buf, count);
}

// ****** HELPER FUNCTIONS ******

static ssize_t show_on_off(bool isOn, char *buf){
  if(isOn){
    strcpy(buf, "on\n");
  }
  else{
    strcpy(buf, "off\n");
  }
  return strlen(buf) + 1;
}

static ssize_t exec_on_off(void (*exec_on)(void), void (*exec_off)(void), const char *buf, size_t count){
  if(!strncmp(buf, "on", 2)) {
    exec_on();
    return 3;
  }
  else if(!strncmp(buf, "off", 3)){
    exec_off();
    return 4;
  }
  return count;
}

static ssize_t show_right_left(bool isRight, char *buf){ 
  if(isRight){
    strcpy(buf, "right\n");
  }
  else{
    strcpy(buf, "left\n");
  }
  return strlen(buf) + 1;
}

static ssize_t exec_right_left(void (*exec_right)(void), void (*exec_left)(void), const char *buf, size_t count){
  if(!strncmp(buf, "right", 2)) {
    exec_right();
    return 6;
  }
  else if(!strncmp(buf, "left", 3)){
    exec_left();
    return 5;
  }
  return count;
}
