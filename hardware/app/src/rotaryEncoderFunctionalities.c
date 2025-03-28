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
#include "hal/rotary_encoder.h"
#include "hal/micHandler.h"
// This will create a thread that constantly monitors rotary encoder inputs and respond appropriately

// Max record for 30 seconds and min for 2 second
#define MAX_RECORDING_DURATION 30
#define MIN_RECORDING_DURATION 2

static pthread_t rotaryEncoderThread;
static bool isInitialized = false;
static bool keepRunning = false;

static int duration = 5; // Default recording duration is 5 seconds

static void* sendRecordingToServer() {
    char* file_path = micHandler_getRecordingPath();
    TCP_sendFileToServer(file_path);
    free(file_path);
}

static void* rotaryEncoderThreadFunc(void* arg)
{
    assert(isInitialized);
    (void)arg;

    while (keepRunning) {        
        // Check for clicks
        if (Rotary_encoder_get_click()) {
            // Start recording with chosen seconds
            printf("Recording %d seconds with mic in process..\n", duration);
            micHandler_startRecording(duration);
            
            // Send the recent recording to TCP server
            sendRecordingToServer();

            // Reset clicked
            Rotary_encoder_set_click(false);
        }

        // Check for rotaryencoder turns
        bool hasTurnedCW = Rotary_encoder_isCW();
        bool hasTurnedCCW = Rotary_encoder_isCCW();
        if(hasTurnedCW && duration < MAX_RECORDING_DURATION) {
            duration += 1;
            printf("Recording duration: %d seconds\n", duration);
        } else if(hasTurnedCCW && duration > MIN_RECORDING_DURATION) {
            duration -= 1;
            printf("Recording duration: %d seconds\n", duration);
        }

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