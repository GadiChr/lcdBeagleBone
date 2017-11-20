
#include "lcdroutines.h"
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/kernel.h>

static struct{
  unsigned char rs; // LOW: command.  HIGH: character.
  unsigned char rw; // LOW: write to LCD.  HIGH: read from LCD.
  unsigned char enable; // activated by a HIGH pulse.
  unsigned char data[8];
} _pin;

static struct{
  unsigned char function;
  unsigned char control;
  unsigned char mode;
} _display;
 
static struct{
  unsigned char row_max;
  unsigned char col_max;
  unsigned char row_offsets[4];
  unsigned char row;
  unsigned char col;
} _cursor;


/****** low level data pushing commands ******/  
static void lcd_send(unsigned char value, unsigned char mode);
static void lcd_write4bits(unsigned char value);
static void lcd_write8bits(unsigned char value);
static void lcd_pulseEnable(void);

/****** div. functions for display initialization ******/
static void lcd_begin(unsigned char cols, unsigned char rows, unsigned char charsize);
static void lcd_setRowOffsets(int row1, int row2, int row3, int row4);

/** 
 *  @brief Initialize the lcd display
 *  @param unsigned char $fourbitmode 
 *  @param unsigned char $rs
 *  @param unsigned char $rw
 *  @param unsigned char $enable
 *  @param unsigned char $d0:$d7
 */
void lcd_init(unsigned char cols, unsigned char lines,
	      unsigned char fourbitmode, unsigned char rs, unsigned char rw, unsigned char enable,
	      unsigned char d0, unsigned char d1, unsigned char d2, unsigned char d3,
	      unsigned char d4, unsigned char d5, unsigned char d6, unsigned char d7){  
  
  _pin.rs = rs;
  _pin.rw = rw;
  _pin.enable = enable;

  _pin.data[0] = d0;
  _pin.data[1] = d1;
  _pin.data[2] = d2;
  _pin.data[3] = d3;
  _pin.data[4] = d4;
  _pin.data[5] = d5;
  _pin.data[6] = d6;
  _pin.data[7] = d7;

  if (fourbitmode) {
    _display.function = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  }
  else {
    _display.function = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
  }
  
  // begin initializing the lcd
  lcd_begin(cols, lines, LCD_5x8DOTS);
}

void lcd_uninit(void){
  int i;

  // clear the display
  lcd_clear();

  // set all gpios to 0
  gpio_set_value(_pin.rs, 0);
  gpio_set_value(_pin.rw, 0);
  gpio_set_value(_pin.enable, 0);

  // unexport all gpios
  gpio_unexport(_pin.rs);
  gpio_unexport(_pin.rw);
  gpio_unexport(_pin.enable);

  // free all gpios
  gpio_free(_pin.rs);
  gpio_free(_pin.rw);
  gpio_free(_pin.enable);

  // do the previous 3 steps for all datapins
  for (i = 0; i<((_display.function & LCD_8BITMODE) ? 8 : 4); i++) {
    gpio_set_value(_pin.data[i], 0);
    gpio_unexport(_pin.data[i]);
    gpio_free(_pin.data[i]);
  }
  
  printk(KERN_INFO "Lcd: all lcd-pins unexported\n");
}

void lcd_begin(unsigned char cols, unsigned char lines, unsigned char dotsize){
  int i = 0;
  if (lines > 1) {
    _display.function |= LCD_2LINE;
  }
  
  _cursor.row_max = lines;
  _cursor.col_max = cols;

  _cursor.row = 0;
  _cursor.col = 0;
  
  lcd_setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);
  
  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
    printk(KERN_INFO "Lcd: character font size = 5x10-Dots\n");
    _display.function |= LCD_5x10DOTS;
  }
  else {
    printk(KERN_INFO "Lcd: caracter font size = 5x8-Dots\n");
  }
  
  // setup rs pin
  gpio_request(_pin.rs, "sysfs");
  gpio_direction_output(_pin.rs, LCD_LOW);
  gpio_export(_pin.rs, false);

  // we can save 1 pin by not using RW. Indicate by passing 255 instead of pin#
  if (_pin.rw != 255) {
    printk(KERN_INFO "Lcd: READ/WRITE pin (RW) is supposed to be driven by gpio%d\n", _pin.rs);
    gpio_request(_pin.rw, "sysfs");
    gpio_direction_output(_pin.rw, LCD_LOW);
    gpio_export(_pin.rw, false);
  }
  else{
    printk(KERN_INFO "Lcd: READ/WRITE pin (RW) is supposed do be connected to ground (GND)\n");
  }

  gpio_request(_pin.enable, "sysfs");
  gpio_direction_output(_pin.enable, LCD_LOW);
  gpio_export(_pin.enable, false);

  // echo all pin connections
  printk(KERN_INFO "Lcd: _pin.rs == %d\n", _pin.rs);
  printk(KERN_INFO "Lcd: _pin.rw == %d\n", _pin.rw);
  printk(KERN_INFO "Lcd: _pin.enable == %d\n", _pin.enable);
  
  // do these once, instead of every time a character is drawn for speed reasons.
  for (i=0; i<((_display.function & LCD_8BITMODE) ? 8 : 4); i++) {
    printk(KERN_INFO "Lcd: _pin.data[%d] == %d\n", i, _pin.data[i]);
    gpio_request(_pin.data[i], "sysfs");
    gpio_direction_output(_pin.data[i], LCD_LOW);
    gpio_export(_pin.data[i], false);
  }

  // see page 45/46 for initialization specificatrion
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // we wait nevertheless
  mdelay(50);

  // pull both RS and R/W low to begin commands
  gpio_set_value(_pin.rs, LCD_LOW);
  gpio_set_value(_pin.enable, LCD_LOW);
  if(_pin.rw != 255){
    gpio_set_value(_pin.rw, LCD_LOW);
  }  

  // put the lcd into 4 bit or 8 bit mode
  if ( !(_display.function & LCD_8BITMODE)) {
    // this is according to the hitachi HD44780 datasheet
    
    // start in 8bit mode, try to set 4 bit mode
    lcd_write4bits(0x03);
    mdelay(5); // wait min 4.1ms

    // second try
    lcd_write4bits(0x03);
    mdelay(5); // wait min 4.1ms

    // third go!
    lcd_write4bits(0x03);
    udelay(150);

    // finally, set to 4-bit interface
    lcd_write4bits(0x02);

    printk(KERN_INFO "Lcd: setup data connection in 4Bit mode\n");
    
  } else {
    // this is according to the hitachi HD44780 datasheet

    // Send function set command sequence
    lcd_command(LCD_FUNCTIONSET | _display.function);
    mdelay(5);  // wait more than 4.1ms

    // second try
    lcd_command(LCD_FUNCTIONSET | _display.function);
    mdelay(5);

    // third go
    lcd_command(LCD_FUNCTIONSET | _display.function);

    printk(KERN_INFO "Lcd: setup data connection in 8Bit mode");
  }

  // finally, set # lines, font size, etc.
  lcd_command(LCD_FUNCTIONSET | _display.function);

  // turn the display on with no cursor or blinking default
  _display.control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  lcd_display();

  // clear it off
  lcd_clear();
  
  // initialize to default text direction (for romance languages)
  _display.mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

  // set the entry mode
  lcd_command(LCD_ENTRYMODESET | _display.mode);
  
}


/***** high level commands ******/

// Control cursor position
void lcd_setCursor(unsigned char col, unsigned char row)
{
  const size_t max_lines = sizeof(_cursor.row_offsets) / sizeof(*_cursor.row_offsets);
  if ( row >= max_lines ) {
    row = max_lines - 1;    // we count rows starting w/0
  }
  if ( row >= _cursor.row_max ) {
    row = _cursor.row_max - 1;    // we count rows starting w/0
  }

  _cursor.col = col;
  _cursor.row = row;
  
  lcd_command(LCD_SETDDRAMADDR | (col + _cursor.row_offsets[row]));
}
unsigned char lcd_getCursorPosRow(void){
  return _cursor.row;
}
unsigned char lcd_getCursorPosCol(void){
  return _cursor.col;
}


// Turn the display on/off (quickly)
void lcd_noDisplay() {
  _display.control &= ~LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | _display.control);
}
void lcd_display(){
  _display.control |= LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | _display.control);
}
bool lcd_isDisplayOn(){
  return (_display.control & LCD_DISPLAYON) ? true : false;
}


// Turns the underline cursor on/off
void lcd_noCursor() {
  _display.control &= ~LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | _display.control);
}
void lcd_cursor() {
  _display.control |= LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | _display.control);
}
bool lcd_isCursorOn(){
  return (_display.control & LCD_CURSORON) ? true : false;
}


// Turn the blinking cursor on/off
void lcd_noBlink() {
  _display.control &= ~LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | _display.control);
}
void lcd_blink() {
  _display.control |= LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | _display.control);
}
bool lcd_isBlinkOn(){
  return (_display.control & LCD_BLINKON) ? true : false;
}

// This will scroll text and keeps the cursor on its column
void lcd_autoscroll(void) {
  _display.mode |= LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | _display.mode);
}
// Instead of scroll text the cursor position gets increased 
void lcd_noAutoscroll(void) {
  _display.mode &= ~LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | _display.mode);
}
bool lcd_isAutoscroll(void){
  return (_display.mode & LCD_ENTRYSHIFTINCREMENT) ? true : false;
}

// These commands scroll the display without changing the RAM
void lcd_scrollDisplayLeft(void) {
  lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void lcd_scrollDisplayRight(void) {
  lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}


// This is for text that flows Left to Right
void lcd_leftToRight(void) {
  _display.mode |= LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | _display.mode);
}
// This is for text that flows Right to Left
void lcd_rightToLeft(void) {
  _display.mode &= ~LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | _display.mode);
}
bool lcd_isLeftToRight(void){
  return (_display.mode & LCD_ENTRYLEFT) ? true : false;
}

// Fill the first 8 CGRAM locations with custom characters
void lcd_createChar(unsigned char location, unsigned char charmap[]) {
  int i;
  location &= 0x7; // we only have 8 locations 0-7
  lcd_command(LCD_SETCGRAMADDR | (location << 3));
  for (i=0; i<8; i++) {
    lcd_write(charmap[i]);
  }
}

void lcd_clear(void){
  lcd_command(LCD_CLEARDISPLAY);   // clear display, set cursor to zero
  mdelay(2);                       // this command takes a long time!
}

void lcd_home(){
  lcd_command(LCD_RETURNHOME);     // set the cursor to zero
  mdelay(2);                       // this command takes a long time!
}

void lcd_print(const char *str){
  lcd_printn((char *)str, strlen(str));
}
void lcd_printn(char *str, size_t n){
  int i = 0;
  while( i < n && str[i] != '\0' ){
    lcd_write(str[i]);
    i++;
  }
}

void lcd_update(char *str){
  lcd_updaten(str, strlen(str));
}

void lcd_updaten(char *str, size_t n){

  // iterate over the entire message
  while (n > 0) {
    // treat escape sequences separately
    if (*str <= 31) {

      switch(*str) {
      case '\e':
	lcd_clear();
	_cursor.row = 0;
	_cursor.col = 0;
	break;
      case '\0':
	_cursor.row = 0;
	_cursor.col = 0;
	lcd_setCursor(_cursor.col, _cursor.row);
	break;
      case '\n':
	_cursor.col = 0;
	_cursor.row++;
	if(_cursor.row >= _cursor.row_max){
	  _cursor.row = 0;
	}
	lcd_setCursor(_cursor.col, _cursor.row);
	break;
      default: break;
      }
      
      str++;
      n--;
      continue;
    }

    // write one character
    lcd_write(*str);
    str++;
    n--;
    
    // jump to next row if line ends
    if (_cursor.col >= _cursor.col_max) {
      _cursor.col = 0;
      _cursor.row++;
      if(_cursor.row >= _cursor.row_max){
	_cursor.row = 0;
      }    
      lcd_setCursor(_cursor.col, _cursor.row);
    }
  }
}

static void lcd_setRowOffsets(int row0, int row1, int row2, int row3){
  _cursor.row_offsets[0] = row0;
  _cursor.row_offsets[1] = row1;
  _cursor.row_offsets[2] = row2;
  _cursor.row_offsets[3] = row3;  
}

/***** mid level commands, for sending data/cmds ******/

void lcd_command(unsigned char value){
  lcd_send(value, LCD_LOW);
}

void lcd_write(unsigned char value){
  _cursor.col++;
  if(_cursor.col >= _cursor.col_max){
    _cursor.col = 0;
    _cursor.row++;
  }
  if(_cursor.row >= _cursor.row_max){
    _cursor.row = 0;
  }
  lcd_send(value, LCD_HIGH);
}

/****** low level data pushing commands ******/

static void lcd_send(unsigned char value, unsigned char mode){
  gpio_set_value(_pin.rs, mode);

  // if there is a RW pin indicated, set it low to Write
  if(_pin.rw != 255){
    gpio_set_value(_pin.rw, LCD_LOW);
  }

  if (_display.function & LCD_8BITMODE) {
    lcd_write8bits(value);
  }
  else {
    lcd_write4bits(value >> 4);
    lcd_write4bits(value);
  }
}

static void lcd_pulseEnable(void){
  gpio_set_value(_pin.enable, LCD_LOW);
  udelay(1);
  gpio_set_value(_pin.enable, LCD_HIGH);
  udelay(2);     // enable pulse must be > 450ns
  gpio_set_value(_pin.enable, LCD_LOW);
  udelay(100);   // commands need > 73us to settle
}

static void lcd_write4bits(unsigned char value){
  int i;
  for (i = 0; i < 4; i++) {
    gpio_set_value(_pin.data[i], LCD_HIGH ? ((value >> i) & 0x01) : (((value >> i) & 0x01) ^ 0x01));
  }
  lcd_pulseEnable();
}
static void lcd_write8bits(unsigned char value){
  int i;
  for (i = 0; i < 8; i++) {
    gpio_set_value(_pin.data[i], LCD_HIGH ? ((value >> i) & 0x01) : (((value >> i) & 0x01) ^ 0x01));
  }
  lcd_pulseEnable();
}
