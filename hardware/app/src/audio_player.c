#include <stdbool.h>
#include <pthread.h>

#include "hal/audioMixer.h"

static bool is_initialized = false;
static pthread_t audioThreadId;
static drumBeat_modes currentMode = NONE;
static double bpm = 0;
static bool pause_beat = false;
static wavedata_t audio;

void* audioThread(void* args){
  while(is_initialized){
    
  }
  return args;
}

void audioPlayer_init(void)
{
  is_initialized = true;
  bpm = DEFAULT_BPM;
  currentMode = ROCK;

  AudioMixer_readWaveFileIntoMemory(BASE, &audio);

  if (pthread_create(&audioThreadId, NULL, audioThread, NULL) != 0) {
    perror("Failed to create Rotary Encoder thread");
    return;
  }
}

void audioPlayer_cleanup(void)
{
  is_initialized = false;
  if (pthread_join(audioThreadId, NULL) != 0) {
    perror("Failed to join sampling thread");
  }

  AudioMixer_freeWaveFileData(&audio);
}

void playRockBeat(){
  double local_bpm = bpm;
  double timeForHalfBeatInSeconds = 60 / local_bpm / 2;
  for(int i=0; i<4; i++){
    if(!pause_beat){
      // Adjust timing if BPM changes mid-beat; breaks under valgrind
      // if(local_bpm != bpm){
      //   local_bpm = bpm;
      //   timeForHalfBeatInSeconds = 60 / local_bpm / 2;
      // }

      AudioMixer_queueSound(&hihat);
      if(i == 0){
        AudioMixer_queueSound(&base);
      }
      if(i == 2){
        AudioMixer_queueSound(&snare);
      }
    }
    sleepForMs(timeForHalfBeatInSeconds * 1000);
  }
}
