/**
 * @file lcdroutines.h
 * @author Christoph Gadinger
 * @date xx.08.2017
 * @brief Header for all relevant LCD routines.
 * This code is adapted for a kernel module and derived from the LiquidCrystal
 * library for Arduino (https://github.com/arduino-libraries/LiquidCrystal.git).
 */

#ifndef _LCDROUTINES_H
#define _LCDROUTINES_H

#include <linux/string.h>
//#include <stdbool.h>

// define logic levels
#define LCD_HIGH false
#define LCD_LOW  (!LCD_HIGH)

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// _display.mode: flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// _display.control: flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// _display.function: flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00


/****** initialization functions ******/
void lcd_init(unsigned char cols, unsigned char lines,
	      unsigned char fourbitmode, unsigned char rs, unsigned char rw, unsigned char enable,
	      unsigned char d0, unsigned char d1, unsigned char d2, unsigned char d3,
	      unsigned char d4, unsigned char d5, unsigned char d6, unsigned char d7);
void lcd_uninit(void);

/****** high level commands, for the user ******/
void lcd_clear(void);
void lcd_home(void);

void lcd_print(const char *str);
void lcd_printn(char *str, size_t n);
void lcd_update(char *str);
void lcd_updaten(char *str, size_t n);

void lcd_noDisplay(void);
void lcd_display(void);
bool lcd_isDisplayOn(void);

void lcd_noBlink(void);
void lcd_blink(void);
bool lcd_isBlinkOn(void);

void lcd_noCursor(void);
void lcd_cursor(void);
bool lcd_isCursorOn(void);

void lcd_scrollDisplayLeft(void);
void lcd_scrollDisplayRight(void);
void lcd_leftToRight(void);
void lcd_rightToLeft(void);
void lcd_autoscroll(void);
void lcd_noAutoscroll(void);
void lcd_createChar(unsigned char, unsigned char[]);

void lcd_setCursor(unsigned char, unsigned char);
unsigned char lcd_getCursorPosRow(void);
unsigned char lcd_getCursorPosCol(void);

/***** mid level commands, for sending data/cmds ******/
void lcd_write(unsigned char);
void lcd_command(unsigned char);

#endif
