// RotaryEncoderFunctionality.h
// Keeps a thread that reads the rotary encoder
// Pressing down changes beat mode
// Knob changes BPM

#ifndef _ROTARYENCODERFUNCTION_H_
#define _ROTARYENCODERFUNCTION_H_

int RotaryEncoderFunction_getDuration(void);
void RotaryEncoderFunction_init(void);
void RotaryEncoderFunction_cleanup(void);

#endif