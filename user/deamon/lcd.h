#pragma once
#ifndef LCD_H
#define LCD_H

void lcd_update(int freq, float temp, int mode);

void lcd_init(void);

void lcd_end(void);

#endif
