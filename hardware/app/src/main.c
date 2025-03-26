// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "hal/accelerometer.h"
#include "hal/gpio.h"
#include "hal/i2c.h"
#include "hal/rotary_encoder.h"
#include "timeFunction.h"
#include "terminal_output.h"
#include "hal/periodTimer.h"
#include "beatboxGenerator.h"
#include "rotaryEncoderFunctionalities.h"
// #include "hal/audioMixer.h"
#include "udp_server.h"
#include "hal/joystick.h"
#include "joystickFunctionalities.h"
#include "lcd_draw.h"
#include "hal/wavePlayback.h"

#include "micHandler.h"

int main()
{
    printf("Starting Program.\n");

    // Initialize all modules; HAL modules first
    Period_init();
    // AudioMixer_init();
    WavePlayback_init();
    Lcd_draw_init();
    Gpio_initialize();
    Rotary_encoder_init();
    // Accelerometer_init();
    Joystick_init();
    // BeatGenerator_init();
    JoystickFunction_init();
    RotaryEncoderFunction_init();
    // TerminalOutput_init();
    UdpServer_start();
    
    // micHandler_startRecording(10);
    

    while(UdpServer_isOnline()) {
        sleep_for_ms(1000);
    }

    printf("Cleaning up modules.\n");

    UdpServer_stop();
    // TerminalOutput_cleanup();
    RotaryEncoderFunction_cleanup();
    JoystickFunction_cleanup();
    // BeatGenerator_cleanup();
    Joystick_cleanup();
    // Accelerometer_cleanup();
    Rotary_encoder_cleanup();
    Gpio_cleanup();
    Lcd_draw_cleanup();
    WavePlayback_cleanup();
    // AudioMixer_cleanup();
    Period_cleanup();




    // snd_pcm_t *handle = WavePlayback_openDevice();

    // wavedata_t sampleFile;
    // WavePlayback_readWaveFileIntoMemory(SOURCE_FILE, &sampleFile);
    // // WavePlayback_playFile(handle, &sampleFile);

    // // WavePlayback_cleanAll(handle, &sampleFile);

    // // Example 2
    // WavePlayback_streamFile(handle, SOURCE_FILE);
    // WavePlayback_cleanAll(handle, &sampleFile);
    // while (1 == 1) {

    // }


    // Let's do have the program run normally, if 


    printf("Program completely successfully.\n");
    return 0;
}