#include "terminal_output.h"
#include "timeFunction.h"
#include "beatboxGenerator.h"
#include "hal/periodTimer.h"
#include "hal/audioMixer.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PRINT_MS_INTERVAL 1000  // Print every 1 second

// Global variables
static pthread_t printThread;
static bool isInitialized = false;
static bool isPrinting = false;

// Add a thread here:
static void* printThreadFunc(void* none)
{
    (void)none; // Suppress unused parameter warning
    while (isPrinting) {
        // Every second, we have to print to the console:
        /*
            Beat mode (its number), format such as "M0"
            Tempo, format such as "90bpm"
            Volume, format such as "vol:80"
            Time between refilling audo playback buffer
                - Each time your code finishes filling the playback buffer, mark the interval/event
                - Format: Audio[{min}, {max}] avg {avg}/{num-samples}
            Time between samples of the accelerometer
                - Each time your code reads the accelerometer, mark an interval/event
                - Format: Accel[{min}, {max}] avg {avg}/{num-samples}
        */
       Period_statistics_t stats;
       Period_statistics_t accelStats;
       Period_getStatisticsAndClear(PERIOD_EVENT_AUDIO_BUFFER, &stats);
       Period_getStatisticsAndClear(PERIOD_EVENT_ACCELEORMETER, &accelStats);

       // Get info to display
       int beatboxMode = BeatGenerator_getMode();
       int bpm = BeatGenerator_getTempo();
       int volume = AudioMixer_getVolume();

       // Display on terminal
        printf("M%d %dbpm vol:%d Audio[%.3f, %.3f] avg %.3f/%d Accel[%.3f, %.3f] avg %.3f/%d\n",
            beatboxMode, bpm, volume,
            stats.minPeriodInMs, stats.maxPeriodInMs, stats.avgPeriodInMs, stats.numSamples,
            accelStats.minPeriodInMs, accelStats.maxPeriodInMs, accelStats.avgPeriodInMs, accelStats.numSamples
            );

        // Sleep for 1 second
        sleep_for_ms(PRINT_MS_INTERVAL);
    }
    return NULL;
}

void TerminalOutput_init(void) 
{
    assert(!isInitialized);
    isInitialized = true;
    isPrinting = true;
    if (pthread_create(&printThread, NULL, printThreadFunc, NULL) != 0) {
        perror("Failed to create print thread");
        exit(EXIT_FAILURE);
    }
    printf("terminal output initalized\n");
}

void TerminalOutput_cleanup(void) 
{
    assert(isInitialized);
    isInitialized = false;
    isPrinting = false;
    pthread_join(printThread, NULL);
    printf("terminal output terminated\n");
}