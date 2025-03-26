// Our file for handling the playing of the .wave file we receive
#ifndef _WAVEPLAYBACK_H_
#define _WAVEPLAYBACK_H_

#include <alsa/asoundlib.h>

#define AUDIOMIXER_MAX_VOLUME 100

typedef struct {
    int numSamples;
    short *pData;
} wavedata_t2;

snd_pcm_t *WavePlayback_openDevice(void);
void WavePlayback_readWaveFileIntoMemory(char *fileName, wavedata_t2 *pWaveStruct);
void WavePlayback_playFile(snd_pcm_t *handle, wavedata_t2 *pWaveData);
void WavePlayback_streamFile(snd_pcm_t *handle, char *fileName);
void WavePlayback_cleanAll(snd_pcm_t *handle, wavedata_t2 *pWaveData);

void WavePlayback_stopPlayback(void);
void WavePlayback_startThread(void);
int WavePlayback_getVolume(void);
void WavePlayback_setVolume(int newVolume);

void WavePlayback_init(void);
void WavePlayback_cleanup(void);

#endif