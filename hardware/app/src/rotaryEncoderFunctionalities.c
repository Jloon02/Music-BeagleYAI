#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

#include "rotaryEncoderFunctionalities.h"
#include "timeFunction.h"
#include "beatboxGenerator.h"
#include "hal/audioMixer.h"
#include "hal/rotary_encoder.h"
// This will create a thread that constantly monitors rotary encoder inputs and respond appropriately

static pthread_t rotaryEncoderThread;
static bool isInitialized = false;
static bool keepRunning = false;

static void* rotaryEncoderThreadFunc(void* arg)
{
    assert(isInitialized);
    (void)arg;

    while (keepRunning) {        
        // Check for clicks
        if (Rotary_encoder_get_click()) {
            // TODO
            // Unknown what we want to do with rotary encoder clicks yet
            Rotary_encoder_set_click(false);
        }

        // Check for tempo changes
        // bool hasTurnedCW = Rotary_encoder_isCW();
        // bool hasTurnedCCW = Rotary_encoder_isCCW();
        // int volume = AudioMixer_getVolume();
        // if(hasTurnedCW) {
        //     volume += 1;
        //     AudioMixer_setVolume(volume);
        // } else if(hasTurnedCCW) {
        //     volume -= 1;
        //     AudioMixer_setVolume(volume);
        // }

        sleep_for_ms(50);
    }
    return NULL;
}

void RotaryEncoderFunction_init(void)
{
    assert(!isInitialized);
    isInitialized = true;
    keepRunning = true;
    // Start thread
    if (pthread_create(&rotaryEncoderThread, NULL,rotaryEncoderThreadFunc, NULL) != 0) {
        perror("Failed to create rotary encoder thread");
        exit(EXIT_FAILURE);
    }
}

void RotaryEncoderFunction_cleanup(void)
{
    assert(isInitialized);
    isInitialized = false;
    keepRunning = false;
    pthread_join(rotaryEncoderThread, NULL);
}