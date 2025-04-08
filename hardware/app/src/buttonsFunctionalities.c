// This watches over buttons and waveplayer, when button gets pressed, this will call waveplayer to skip or backtrack
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>      //printf()
#include <stdlib.h>     //exit()
#include <string.h>
#include <stdbool.h>
#include <unistd.h>     //usleep()

#include "hal/buttons.h"
#include "hal/wavePlayback.h"

static bool isInitialized = false;
static pthread_t button_check_thread;
static bool keepRunning = false;

static void* button_check_function(void* arg)
{
    (void)arg; // Unused parameter
    
    while (keepRunning) {
        // Check right button
        if (Buttons_get_right()) {
            printf("Right button clicked!\n");
            WavePlayback_jumpForward();
            Buttons_set_right(false); // Reset the click state
        }
        
        // Check left button
        if (Buttons_get_left()) {
            printf("Left button clicked!\n");
            WavePlayback_jumpBackward();
            Buttons_set_left(false); // Reset the click state
        }
        
        // Small delay to prevent busy-waiting
        usleep(10000); // 10ms
    }
    
    return NULL;
}

void ButtonFunctionalities_init(void)
{
    assert(!isInitialized);
    
    // Initialize buttons subsystem if not already done
    Buttons_init();
    
    keepRunning = true;
    if (pthread_create(&button_check_thread, NULL, button_check_function, NULL) != 0) {
        perror("Failed to create button check thread");
        exit(EXIT_FAILURE);
    }
    
    isInitialized = true;
}

void ButtonFunctionalities_cleanup(void)
{
    assert(isInitialized);
    
    // Signal thread to stop
    keepRunning = false;
    
    // Wait for thread to finish
    pthread_join(button_check_thread, NULL);
    
    // Cleanup buttons subsystem
    Buttons_cleanup();
    
    isInitialized = false;
}