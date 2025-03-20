// beatboxGenerator.h
// Tracks the current beat mode
// Plays sounds at the correct intervals based on tempo

#ifndef _BEATBOX_GENERATOR_H
#define _BEATBOX_GENERATOR_H

#define MAX_TEMPO 300
#define MIN_TEMPO 40

typedef enum {
    BEAT_NONE,
    BEAT_ROCK,
    BEAT_CUSTOM
} BeatMode;

void BeatGenerator_cleanup();
void BeatGenerator_init();
const char* BeatMode_toString(BeatMode mode);
void BeatGenerator_setMode(BeatMode mode);
void BeatGenerator_setTempo(int newTempo);
BeatMode BeatGenerator_getMode();
int BeatGenerator_getTempo();

// Playing different sounds
void BeatGenerator_playDrum();
void BeatGenerator_playHat();
void BeatGenerator_playSnare();

#endif