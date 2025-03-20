#include "beatboxGenerator.h"
#include "hal/accelerometer.h"
#include "hal/audioMixer.h"
#include "hal/rotary_encoder.h"
#include "timeFunction.h"
#include "hal/periodTimer.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>

// Drum sounds
#define BASE_DRUM "beatbox-wave-files/100051__menegass__gui-drum-bd-hard.wav"
#define SNARE "beatbox-wave-files/100059__menegass__gui-drum-snare-soft.wav"
#define HI_HAT "beatbox-wave-files/100053__menegass__gui-drum-cc.wav"

static BeatMode currentBeatMode = BEAT_ROCK; // Default to Rock beat
static int tempo = 120; // Default tempo (BPM)
static wavedata_t baseDrum, hiHat, snare;

// Thread for beatbox sound generation
static pthread_t beatboxThread;
static pthread_t airdrumThread;
static pthread_mutex_t beatboxMutex = PTHREAD_MUTEX_INITIALIZER;
static bool isThreadRunning = false;
static bool isairdrumThreadRunning = false;
static bool isInitialized = false;

void BeatGenerator_playDrum() {
    AudioMixer_queueSound(&baseDrum); 
}

void BeatGenerator_playHat() {
    AudioMixer_queueSound(&hiHat);
}

void BeatGenerator_playSnare() {
    AudioMixer_queueSound(&snare);
}

// Function to play the Rock beat
static void playRockBeat() {
    BeatGenerator_playHat(); // Hi-hat on every half beat
    static int beatCount = 0;
    beatCount++;

    switch (beatCount % 8) {
        case 0: // Beat 1
        case 4: // Beat 3
            BeatGenerator_playDrum(); // Base drum on beats 1 and 3
            break;
        case 2: // Beat 2
        case 6: // Beat 4
            BeatGenerator_playSnare(); // Snare on beats 2 and 4
            break;
    }
}

// Function to play the Custom beat
static void playCustomBeat() {
    static int beatCount = 0;
    beatCount++;

    switch (beatCount % 8) {
        case 0: // Beat 1
            BeatGenerator_playDrum();
            break;
        case 2: // Beat 2
            BeatGenerator_playHat();
            break;
        case 4: // Beat 3
            BeatGenerator_playSnare(); // Base drum on beats 1 and 3
            break;
        case 6: // Beat 4
            BeatGenerator_playDrum(); // Snare on beats 2 and 4
            BeatGenerator_playHat();
            break;
    }
}


// Thread function to generate beats
void* beatThread(void* arg) {
    (void)arg; //ignore warning
    while (isThreadRunning) {
        // Continously repeat beat
        pthread_mutex_lock(&beatboxMutex);
        switch (currentBeatMode) {
            case BEAT_NONE:
                // No sound
                break;
            case BEAT_ROCK:
                playRockBeat();
                break;
            case BEAT_CUSTOM:
                playCustomBeat();
                break;
        }
        pthread_mutex_unlock(&beatboxMutex);

        // Wait for half a beat
        sleep_for_ms(((60.0 / tempo / 2.0) * 1000));
    }
    return NULL;
}

// Consider creating a new .c file that does the air drumming
void* airdrumGeneratorThread(void *arg) {
    (void)arg;
    while (isairdrumThreadRunning) {
        if (accelerometer_x_sound()) {
            BeatGenerator_playSnare();
        }
        if (accelerometer_y_sound()) {
            BeatGenerator_playHat();
        }
        if (accelerometer_z_sound()) {
            BeatGenerator_playDrum();
        }

        // Mark stats about acceleormeter
        Period_markEvent(PERIOD_EVENT_ACCELEORMETER);
    }
    return NULL;
}

// Function to load drum sounds
static void BeatGenerator_loadSounds() {
    AudioMixer_readWaveFileIntoMemory(BASE_DRUM, &baseDrum);
    AudioMixer_readWaveFileIntoMemory(HI_HAT, &hiHat);
    AudioMixer_readWaveFileIntoMemory(SNARE, &snare);
}

// Init function for Beatbox
void BeatGenerator_init() {
    assert(!isInitialized);
    assert(!isThreadRunning);

    BeatGenerator_loadSounds();
    isThreadRunning = true;
    isairdrumThreadRunning = true;
    isInitialized = true;

    if (pthread_create(&beatboxThread, NULL, beatThread, NULL) != 0) {
        perror("Failed to create beatboxGenerator thread");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&airdrumThread, NULL, airdrumGeneratorThread, NULL) != 0) {
        perror("Failed to create airdrumGenerator thread");
        exit(EXIT_FAILURE);
    } 

}


// Function to free drum sounds
void BeatGenerator_cleanup() {
    assert(isInitialized);

    isThreadRunning = false;
    isairdrumThreadRunning = false;
    pthread_join(beatboxThread, NULL);
    pthread_join(airdrumThread, NULL);
    pthread_mutex_destroy(&beatboxMutex);

    // Free all the audio files
    AudioMixer_freeWaveFileData(&baseDrum);
    AudioMixer_freeWaveFileData(&hiHat);
    AudioMixer_freeWaveFileData(&snare);

    isInitialized = false;
}

const char* BeatMode_toString(BeatMode mode) {
    switch (mode) {
        case BEAT_NONE:   return "Beat: None";
        case BEAT_ROCK:   return "Beat: Rock";
        case BEAT_CUSTOM: return "Beat: Rock 2";
        default:          return "Beat: Unknown";
    }
}

// Function to set the beat mode
void BeatGenerator_setMode(BeatMode mode) {
    pthread_mutex_lock(&beatboxMutex);
    currentBeatMode = mode;
    pthread_mutex_unlock(&beatboxMutex);
}

// Function to set the tempo
void BeatGenerator_setTempo(int newTempo) {
    if (newTempo < MIN_TEMPO || newTempo > MAX_TEMPO) {
        printf("ERROR: Tempo must be between 40 and 300 BPM.\n");
        return;
    }
    pthread_mutex_lock(&beatboxMutex);
    tempo = newTempo;
    pthread_mutex_unlock(&beatboxMutex);
}

// Function to get the current beat mode
BeatMode BeatGenerator_getMode() {
    return currentBeatMode;
}

// Function to get the current tempo
int BeatGenerator_getTempo() {
    return tempo;
}