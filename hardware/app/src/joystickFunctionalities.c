#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

#include "hal/timeFunction.h"
#include "hal/wavePlayback.h"
#include "hal/joystick.h"
#include "lcd_draw.h"
#include "joystickFunctionalities.h"
#include "song_metadata.h"
#include "shutdown.h"

// This will create a thread that constantly monitors joystick inputs and respond appropriately
static pthread_t joystickThread;
static bool isInitialized = false;
static bool keepRunning = false;

// Variables for left/right delay control
static long long lastLeftTime = 0;
static long long lastRightTime = 0;
static const long long LEFT_RIGHT_DELAY_MS = 500; // 0.5 second delay

// Variables for button hold shutdown delay
static const long long SHUTDOWN_HOLD_TIME_MS = 2000; // 2 seconds to shutdown

static void* joystick_running(void* arg)
{
    assert(isInitialized);
    (void)arg;
    while (keepRunning) {
        Direction current = get_direction();
        int volume = WavePlayback_getVolume();
        long long currentTime = get_time_in_ms();
        
        if (current == DIR_UP && volume < AUDIOMIXER_MAX_VOLUME) {
            WavePlayback_setVolume(volume + 5);
        } else if (current == DIR_DOWN && volume > 0) {
            WavePlayback_setVolume(volume - 5);
        }
        else if (current == DIR_RIGHT && (currentTime - lastRightTime) > LEFT_RIGHT_DELAY_MS) {
            SongMetadata_nextSong();
            lastRightTime = currentTime;
        }
        else if (current == DIR_LEFT && (currentTime - lastLeftTime) > LEFT_RIGHT_DELAY_MS) {
            SongMetadata_previousSong();
            lastLeftTime = currentTime;
        }

        // Handle button click (short press)
        if (joystick_button_clicked()) {
            SongMetadata_togglePlay();
        }

        // Handle button hold (shutdown)
        if (joystick_button_is_held_for_ms(SHUTDOWN_HOLD_TIME_MS)) {
            Shutdown_requestShutdown();
            printf("Shutting down due to button hold...\n");
        }
        
        
        sleep_for_ms(50);
    }
    return NULL;
}


void JoystickFunction_init(void)
{
    assert(!isInitialized);
    isInitialized = true;
    keepRunning = true;
    lastLeftTime = 0;
    lastRightTime = 0;

    // Start thread
    if (pthread_create(&joystickThread, NULL,joystick_running, NULL) != 0) {
        perror("Failed to create rotary encoder thread");
        exit(EXIT_FAILURE);
    }
}

void JoystickFunction_cleanup(void)
{
    assert(isInitialized);
    isInitialized = false;
    keepRunning = false;
    pthread_join(joystickThread, NULL);
}