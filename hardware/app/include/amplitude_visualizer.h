// r5_led.h
// provides led signal to R5 based on amplitude of song
// communicate with the R5 through shared data

#ifndef AMPLITUDE_VISUALIZER_H
#define AMPLITUDE_VISUALIZER_H

// Initializes and creates game thread
void AmplitudeVisualizer_init(void);

// Cleans up initialized anad joins thread
void AmplitudeVisualizer_cleanup(void);

#endif