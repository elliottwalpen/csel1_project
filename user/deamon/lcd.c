#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <syslog.h>

#include "ssd1306.h"
#include "lcd.h"
#include "fan.h"

int lcd_nbdigit(int nb); //Get the nuner of digit contained in an integer

void lcd_update(int freq, float temp, int mode)
{
    char *msg_freq[(strlen("Temp: 'C  ")+lcd_nbdigit(temp))*sizeof(char)];
    char *msg_temp[(strlen("Freq: Hz  ")+lcd_nbdigit(freq))*sizeof(char)];

    sprintf(msg_temp, "Temp: %.1f'C  ", temp);
    sprintf(msg_freq, "Freq: %dHz  ", freq);

    //Print titles
    ssd1306_set_position (0,0);
    ssd1306_puts("CSEL1a - SP.07");
    ssd1306_set_position (0,1);
    ssd1306_puts("  Demo - SW");
    ssd1306_set_position (0,2);
    ssd1306_puts("--------------");

    //Print data
    ssd1306_set_position (0,3);
    ssd1306_puts(msg_temp);
    ssd1306_set_position (0,4);
    ssd1306_puts(msg_freq);

    //print mode
	if(mode == MANUAL){
		ssd1306_set_position (0,5);
    	ssd1306_puts("Mode: Manual");
	}else if(mode == AUTO){
		ssd1306_set_position (0,5);
    	ssd1306_puts("Mode: Auto  ");
    }
}

int lcd_nbdigit(int nb)
{
    int cnt = 0;
    while (nb != 0)
    {
        nb /= 10;
        cnt++;
    }
    return cnt;
}

void lcd_init()
{
    ssd1306_init();

    lcd_update(driver_get_freq(), driver_get_temperature(), driver_get_mode());
}

void lcd_end()
{
    ssd1306_clear_display();
}