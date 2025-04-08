#include "shutdown.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// Include model init/cleanups
#include "hal/gpio.h"
#include "hal/i2c.h"
#include "hal/rotary_encoder.h"
#include "hal/joystick.h"
#include "hal/micHandler.h"
#include "hal/wavePlayback.h"
#include "buttonsFunctionalities.h"

#include "rotaryEncoderFunctionalities.h"
#include "timeFunction.h"
#include "lcd_draw.h"
#include "joystickFunctionalities.h"
#include "tcp_server.h"
#include "amplitude_visualizer.h"
#include "song_metadata.h"

static bool shutdownRequested = false;
static pthread_mutex_t shutdownMutex = PTHREAD_MUTEX_INITIALIZER;

void Shutdown_init(void) {
    pthread_mutex_lock(&shutdownMutex);
    shutdownRequested = false;
    pthread_mutex_unlock(&shutdownMutex);

    // Read files saved on device
    SongMetadata_readMetadataFile("MusicBoard-audio-files/saved_metadata");

    // Init all modules
    WavePlayback_init();
    AmplitudeVisualizer_init();
    Lcd_draw_init();
    Gpio_initialize();
    Rotary_encoder_init();
    Joystick_init();
    ButtonFunctionalities_init();
    JoystickFunction_init();
    RotaryEncoderFunction_init();
}

void Shutdown_cleanup(void) {
    // Handle cleanup of all modules
    RotaryEncoderFunction_cleanup();
    JoystickFunction_cleanup();
    ButtonFunctionalities_cleanup();
    Joystick_cleanup();
    Rotary_encoder_cleanup();
    Gpio_cleanup();
    Lcd_draw_cleanup();
    AmplitudeVisualizer_cleanup();
    WavePlayback_cleanup();
}

void Shutdown_requestShutdown(void) {
    pthread_mutex_lock(&shutdownMutex);
    shutdownRequested = true;
    pthread_mutex_unlock(&shutdownMutex);
    
    printf("Shutdown requested.\n");
}

bool Shutdown_isShutdownRequested(void) {
    pthread_mutex_lock(&shutdownMutex);
    bool requested = shutdownRequested;
    pthread_mutex_unlock(&shutdownMutex);
    return requested;
}