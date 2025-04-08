// Our file for handling the playing of the .wave file we receive
#ifndef _WAVEPLAYBACK_H_
#define _WAVEPLAYBACK_H_

#include <alsa/asoundlib.h>
#include <stdbool.h>

#define AUDIOMIXER_MAX_VOLUME 100

typedef struct {
    int numSamples;
    short *pData;
} wavedata_t;

int WavePlayback_getVolume(void);
void WavePlayback_setVolume(int newVolume);
bool WavePlayback_isPlaying(void);
bool WavePlayback_isPaused(void);
float WavePlayback_getCurrentAmplitude(void);

void WavePlayback_init(void);
void WavePlayback_cleanup(void);

void WavePlayback_startThread(const char* path);
void WavePlayback_stopPlayback(void);
void WavePlayback_pausePlayback(void);
void WavePlayback_resumePlayback(void);

#endif