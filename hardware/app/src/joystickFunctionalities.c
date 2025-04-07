#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

#include "timeFunction.h"
#include "hal/wavePlayback.h"
#include "hal/joystick.h"
#include "lcd_draw.h"
#include "joystickFunctionalities.h"
#include "song_metadata.h"
// This will create a thread that constantly monitors joystick inputs and respond appropriately

static pthread_t joystickThread;
static bool isInitialized = false;
static bool keepRunning = false;
static bool playSong = false;

static void* joystick_running(void* arg)
{
    assert(isInitialized);
    (void)arg;
    // Lcd_draw_songScreen();
    while (keepRunning) {
        Direction current = get_direction();
        int volume = WavePlayback_getVolume();
        
        if (current == DIR_UP && volume < AUDIOMIXER_MAX_VOLUME) {
            WavePlayback_setVolume(volume + 5);
        } else if (current == DIR_DOWN && volume > 0) {
            WavePlayback_setVolume(volume - 5);
        }
        else if (current == DIR_RIGHT) {
            SongMetadata_nextSong();
        }
        else if (current == DIR_LEFT) {
            SongMetadata_previousSong();
        }

        if(joystick_button_clicked()){
            SongMetadata_togglePlay();
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