#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "r5_shared_data.h"
#include "hal/wavePlayback.h"
#include "amplitude_visualizer.h"

// General R5 Memory Sharing Routine
// ----------------------------------------------------------------
#define ATCM_ADDR     0x79000000  // MCU ATCM (p59 TRM)
#define BTCM_ADDR     0x79020000  // MCU BTCM (p59 TRM)
#define MEM_LENGTH    0x8000

static pthread_t AmplitudeVisualizer_threadId;
static bool isInitialized = false;
static bool isRunning = false;

// Amplitude visualization parameters
#define AMPLITUDE_SCALE_FACTOR 15   // Scale amplitude to LED range
#define SMOOTHING_FACTOR 0.3       // For smoothing amplitude changes
#define POLL_INTERVAL_MS 10        // How often to check amplitude (ms)
#define DECAY_FACTOR 0.9f          // Fading out speed when song is stopped

static float smoothedAmplitude = 0.0f;

static void* AmplitudeVisualizer_thread(void* arg);

// Return the address of the base address of the ATCM memory region for the R5-MCU
volatile void* getR5MmapAddr(void)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("ERROR: could not open /dev/mem; Did you run with sudo?");
        exit(EXIT_FAILURE);
    }

    volatile void* pR5Base = mmap(0, MEM_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, BTCM_ADDR);
    if (pR5Base == MAP_FAILED) {
        perror("ERROR: could not map memory");
        exit(EXIT_FAILURE);
    }
    close(fd);

    return pR5Base;
}

void freeR5MmapAddr(volatile void* pR5Base)
{
    if (munmap((void*) pR5Base, MEM_LENGTH)) {
        perror("R5 munmap failed");
        exit(EXIT_FAILURE);
    }
}

void AmplitudeVisualizer_init(void) {
    assert(!isInitialized);
    isInitialized = true;
    isRunning = true;

    // Start visualization thread
    if (pthread_create(&AmplitudeVisualizer_threadId, NULL, 
                      AmplitudeVisualizer_thread, NULL) != 0) {
        perror("Failed to create amplitude visualization thread");
        exit(EXIT_FAILURE);
    }
}

void AmplitudeVisualizer_cleanup(void) {
    assert(isInitialized);

    // Signal the thread to stop
    isRunning = false;
    
    // Wait for the thread to finish
    if (pthread_join(AmplitudeVisualizer_threadId, NULL) != 0) {
        perror("Failed to join amplitude visualization thread\n");
    }

    // Reset state variables
    isInitialized = false;
    smoothedAmplitude = 0.0f;
    
    printf("Amplitude visualizer cleaned up successfully\n");
}

void* AmplitudeVisualizer_thread(void *arg) {
    (void) arg;
        
    volatile uint8_t* pR5Base = getR5MmapAddr();
    
    while (isRunning) {
        int ledPosition = 0;

        if (WavePlayback_isPlaying() && !WavePlayback_isPaused()) {
            // Get current amplitude through polling
            float currentAmplitude = WavePlayback_getCurrentAmplitude();
            
            // Apply smoothing
            smoothedAmplitude = SMOOTHING_FACTOR * currentAmplitude + 
                                (1.0 - SMOOTHING_FACTOR) * smoothedAmplitude;
            
            // Convert smoothed amplitude to LED position (0-15)
            ledPosition = (int)(smoothedAmplitude * AMPLITUDE_SCALE_FACTOR);
        } else {
            // If not playing, gradually decrease the amplitude
            if (smoothedAmplitude > 0.0f) {
                smoothedAmplitude *= DECAY_FACTOR;  // Slowly decay the amplitude
                if (smoothedAmplitude < 0.01f) {    // Small threshold to prevent never reaching 0
                    smoothedAmplitude = 0.0f;
                }
            }
            ledPosition = (int)(smoothedAmplitude * AMPLITUDE_SCALE_FACTOR);
        }

        // Ensure the value stays within bounds
        if (ledPosition < 0) ledPosition = 0;
        if (ledPosition > AMPLITUDE_SCALE_FACTOR) ledPosition = AMPLITUDE_SCALE_FACTOR;

        // Write the amplitude value to R5 memory
        MEM_UINT32(pR5Base + AMP_OFFSET) = ledPosition;
        
        // Small delay before next poll
        usleep(POLL_INTERVAL_MS * 1000);
    }
    
    // Clean up when exiting
    freeR5MmapAddr(pR5Base);
    return NULL;
}

/*
ADD THIS PART IF CALLING WAVEPLAYBACK THREAD WITH AUDIO FILEPATH
*/

// Function to start music playback with visualization
// void AmplitudeVisualizer_start(const char* audioFilePath)
// {
//     if (!isRunning) {
//         AmplitudeVisualizer_init();
//         WavePlayback_startThread(audioFilePath);
//     }
// }

// Function to stop music visualization
// void AmplitudeVisualizer_stop()
// {
//     if (isRunning) {
//         WavePlayback_stopPlayback();
//         AmplitudeVisualizer_cleanup();
//     }
// }