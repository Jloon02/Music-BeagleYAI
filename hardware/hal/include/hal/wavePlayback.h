// Our file for handling the playing of the .wave file we receive
#ifndef _WAVEPLAYBACK_H_
#define _WAVEPLAYBACK_H_

#include <alsa/asoundlib.h>

#define AUDIOMIXER_MAX_VOLUME 100

typedef struct {
    int numSamples;
    short *pData;
} wavedata_t;

void WavePlayback_startThread(const char* path);
int WavePlayback_getVolume(void);
void WavePlayback_setVolume(int newVolume);
float WavePlayback_getCurrentAmplitude(void);

void WavePlayback_init(void);
void WavePlayback_cleanup(void);

#endif