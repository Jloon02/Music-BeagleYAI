#ifndef _BUTTONS_H_
#define _BUTTONS_H_

bool Buttons_get_right(void);
bool Buttons_get_left(void);
void Buttons_set_right(bool status);
void Buttons_set_left(bool status);
void Buttons_init(void);
void Buttons_cleanup(void);

#endif