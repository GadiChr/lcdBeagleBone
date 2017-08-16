
#include "lcdroutines.h"
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/kernel.h>

unsigned char _rs_pin; // LOW: command.  HIGH: character.
unsigned char _rw_pin; // LOW: write to LCD.  HIGH: read from LCD.
unsigned char _enable_pin; // activated by a HIGH pulse.
unsigned char _data_pins[8];

unsigned char _displayfunction;
unsigned char _displaycontrol;
unsigned char _displaymode;

unsigned char _initialized;

unsigned char _numlines;
unsigned char _numcols;
unsigned char _row_offsets[4];

unsigned char _currentrow = 0;
unsigned char _currentcol = 0;

void lcd_init(unsigned char fourbitmode, unsigned char rs, unsigned char rw, unsigned char enable,
	  unsigned char d0, unsigned char d1, unsigned char d2, unsigned char d3,
	  unsigned char d4, unsigned char d5, unsigned char d6, unsigned char d7){  
  
  _rs_pin = rs;
  _rw_pin = rw;
  _enable_pin = enable;

  _data_pins[0] = d0;
  _data_pins[1] = d1;
  _data_pins[2] = d2;
  _data_pins[3] = d3;
  _data_pins[4] = d4;
  _data_pins[5] = d5;
  _data_pins[6] = d6;
  _data_pins[7] = d7;

  if (fourbitmode) {
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  }
  else {
    _displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
  }
  
  // begin initializing the lcd
  lcd_begin(20, 2, LCD_5x8DOTS);
}

void lcd_uninit(void){
  int i;

  // clear the display
  lcd_clear();

  // set all gpios to 0
  gpio_set_value(_rs_pin, 0);
  gpio_set_value(_rw_pin, 0);
  gpio_set_value(_enable_pin, 0);

  // unexport all gpios
  gpio_unexport(_rs_pin);
  gpio_unexport(_rw_pin);
  gpio_unexport(_enable_pin);

  // free all gpios
  gpio_free(_rs_pin);
  gpio_free(_rw_pin);
  gpio_free(_enable_pin);

  // do the previous 3 steps for all datapins
  for (i = 0; i<((_displayfunction & LCD_8BITMODE) ? 8 : 4); i++) {
    gpio_set_value(_data_pins[i], 0);
    gpio_unexport(_data_pins[i]);
    gpio_free(_data_pins[i]);
  }
  
  printk(KERN_INFO "Lcd: all lcd-pins unexported\n");
}

void lcd_begin(unsigned char cols, unsigned char lines, unsigned char dotsize){
  int i = 0;
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;
  _numcols = cols;
  
  lcd_setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);
  
  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
    printk(KERN_INFO "Lcd: character font size = 5x10-Dots\n");
    _displayfunction |= LCD_5x10DOTS;
  }
  else {
    printk(KERN_INFO "Lcd: caracter font size = 5x8-Dots\n");
  }
  
  // setup rs pin
  gpio_request(_rs_pin, "sysfs");
  gpio_direction_output(_rs_pin, LCD_LOW);
  gpio_export(_rs_pin, false);

  // we can save 1 pin by not using RW. Indicate by passing 255 instead of pin#
  if (_rw_pin != 255) {
    printk(KERN_INFO "Lcd: READ/WRITE pin (RW) is supposed to be driven by gpio%d\n", _rs_pin);
    gpio_request(_rw_pin, "sysfs");
    gpio_direction_output(_rw_pin, LCD_LOW);
    gpio_export(_rw_pin, false);
  }
  else{
    printk(KERN_INFO "Lcd: READ/WRITE pin (RW) is supposed do be connected to ground (GND)\n");
  }

  gpio_request(_enable_pin, "sysfs");
  gpio_direction_output(_enable_pin, LCD_LOW);
  gpio_export(_enable_pin, false);

  // echo all pin connections
  printk(KERN_INFO "Lcd: _rs_pin == %d\n", _rs_pin);
  printk(KERN_INFO "Lcd: _rw_pin == %d\n", _rw_pin);
  printk(KERN_INFO "Lcd: _enable_pin == %d\n", _enable_pin);
  
  // do these once, instead of every time a character is drawn for speed reasons.
  for (i=0; i<((_displayfunction & LCD_8BITMODE) ? 8 : 4); i++) {
    printk(KERN_INFO "Lcd: _data_pins[%d] == %d\n", i, _data_pins[i]);
    gpio_request(_data_pins[i], "sysfs");
    gpio_direction_output(_data_pins[i], LCD_LOW);
    gpio_export(_data_pins[i], false);
  }

  // see page 45/46 for initialization specificatrion
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // we wait nevertheless
  mdelay(50);

  // now we pull both RS and R/W low to begin commands
  gpio_set_value(_rs_pin, LCD_LOW);
  gpio_set_value(_enable_pin, LCD_LOW);
  if(_rw_pin != 255){
    gpio_set_value(_rw_pin, LCD_LOW);
  }  

  // put the lcd into 4 bit or 8 bit mode
  if ( !(_displayfunction & LCD_8BITMODE)) {
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
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
    // page 45 figure 23

    // Send function set command sequence
    lcd_command(LCD_FUNCTIONSET | _displayfunction);
    mdelay(5);  // wait more than 4.1ms

    // second try
    lcd_command(LCD_FUNCTIONSET | _displayfunction);
    mdelay(5);

    // third go
    lcd_command(LCD_FUNCTIONSET | _displayfunction);

    printk(KERN_INFO "Lcd: setup data connection in 8Bit mode");
  }

  // finally, set # lines, font size, etc.
  lcd_command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  lcd_display();

  // clear it off
  lcd_clear();
  
  // initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  lcd_command(LCD_ENTRYMODESET | _displaymode);
  
}

/***** high level commands, for the user ******/

void lcd_setCursor(uint8_t col, uint8_t row)
{
  const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
  if ( row >= max_lines ) {
    row = max_lines - 1;    // we count rows starting w/0
  }
  if ( row >= _numlines ) {
    row = _numlines - 1;    // we count rows starting w/0
  }

  _currentrow = row;
  
  lcd_command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
}

// Turn the display on/off (quickly)
void lcd_noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void lcd_noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_cursor() {
  _displaycontrol |= LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void lcd_noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd_blink() {
  _displaycontrol |= LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
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
  _displaymode |= LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void lcd_rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void lcd_autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void lcd_noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
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
  mdelay(2);                    // this command takes a long time!
}

void lcd_home(){
  lcd_command(LCD_RETURNHOME);     // set the cursor to zero
  mdelay(2);                    // this command takes a long time!
}

void lcd_display(){
  _displaycontrol |= LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
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
	_currentrow = 0;
	_currentcol = 0;
	break;
      case '\0':
	_currentrow = 0;
	_currentcol = 0;
	lcd_setCursor(_currentcol, _currentrow);
	break;
      case '\n':
	_currentcol = 0;
	_currentrow++;
	if(_currentrow >= _numlines){
	  _currentrow = 0;
	}
	lcd_setCursor(_currentcol, _currentrow);
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
    if (_currentcol >= _numcols) {
      _currentcol = 0;
      _currentrow++;
      if(_currentrow >= _numlines){
	_currentrow = 0;
      }
      
      lcd_setCursor(_currentcol, _currentrow);
    }
  }
}

void lcd_setRowOffsets(int row0, int row1, int row2, int row3){
  _row_offsets[0] = row0;
  _row_offsets[1] = row1;
  _row_offsets[2] = row2;
  _row_offsets[3] = row3;  
}

/***** mid level commands, for sending data/cmds ******/

void lcd_command(unsigned char value){
  lcd_send(value, LCD_LOW);
}

void lcd_write(unsigned char value){
  _currentcol++;
  lcd_send(value, LCD_HIGH);
}

/****** low level data pushing commands ******/

void lcd_send(unsigned char value, unsigned char mode){
  gpio_set_value(_rs_pin, mode);

  // if there is a RW pin indicated, set it low to Write
  if(_rw_pin != 255){
    gpio_set_value(_rw_pin, LCD_LOW);
  }

  if (_displayfunction & LCD_8BITMODE) {
    lcd_write8bits(value);
  }
  else {
    lcd_write4bits(value >> 4);
    lcd_write4bits(value);
  }
}

void lcd_pulseEnable(void){
  gpio_set_value(_enable_pin, LCD_LOW);
  udelay(1);
  gpio_set_value(_enable_pin, LCD_HIGH);
  udelay(2);     // enable pulse must be > 450ns
  gpio_set_value(_enable_pin, LCD_LOW);
  udelay(100);   // commands need > 73us to settle
}

void lcd_write4bits(unsigned char value){
  int i;
  for (i = 0; i < 4; i++) {
    gpio_set_value(_data_pins[i], LCD_HIGH ? ((value >> i) & 0x01) : (((value >> i) & 0x01) ^ 0x01));
  }
  lcd_pulseEnable();
}
void lcd_write8bits(unsigned char value){
  int i;
  for (i = 0; i < 8; i++) {
    gpio_set_value(_data_pins[i], LCD_HIGH ? ((value >> i) & 0x01) : (((value >> i) & 0x01) ^ 0x01));
  }
  lcd_pulseEnable();
}
