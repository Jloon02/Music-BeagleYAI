// rotary_encoder.h
// This should handle the logic for detecting rotation, processes A and B signals

#ifndef _ROTARYENCODER_H_
#define _ROTARYENCODER_H_

#include <stdbool.h>
void Rotary_encoder_init(void);
void Rotary_encoder_cleanup(void);

// There should be a function that helps return what the current count is, decoupled from emitter
bool Rotary_encoder_get_click(void);
void Rotary_encoder_set_click(bool status);
bool Rotary_encoder_isCW(void);
bool Rotary_encoder_isCCW(void);

#endif