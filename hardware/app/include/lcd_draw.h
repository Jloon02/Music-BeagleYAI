/*
    lcd_draw is responsible for printing out the screen and keeping
    track of which screen is on
*/
#ifndef _LCD_DRAW_H_
#define _LCD_DRAW_H_

void Lcd_draw_init(void);
void Lcd_draw_cleanup(void);

void Lcd_draw_songScreen(void);

int Lcd_get_screen(void);
void Lcd_set_screen(void);


#endif