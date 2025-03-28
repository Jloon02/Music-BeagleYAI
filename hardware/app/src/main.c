// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "hal/gpio.h"
#include "hal/i2c.h"
#include "hal/rotary_encoder.h"
#include "timeFunction.h"
#include "rotaryEncoderFunctionalities.h"
// #include "hal/audioMixer.h"
#include "hal/joystick.h"
#include "joystickFunctionalities.h"
#include "lcd_draw.h"
#include "hal/wavePlayback.h"

#include "hal/micHandler.h"
#include "tcp_server.h"

int main()
{
    printf("Starting Program.\n");

    // Initialize all modules; HAL modules first
    WavePlayback_init();
    Lcd_draw_init();
    Gpio_initialize();
    Rotary_encoder_init();
    Joystick_init();
    JoystickFunction_init();
    RotaryEncoderFunction_init();
    TCP_sendFileToServer("wave-files/nggyucut.wav");
    while (1 == 1) {
        
    }
    RotaryEncoderFunction_cleanup();
    JoystickFunction_cleanup();
    Joystick_cleanup();
    Rotary_encoder_cleanup();
    Gpio_cleanup();
    Lcd_draw_cleanup();
    WavePlayback_cleanup();
    
    
    // start mic then send
    // micHandler_startRecording(5);
    // printf("main has finished recording\n");

    // char* file_path = micHandler_getRecordingPath();
    // TCP_sendFileToServer(file_path);
    // free(file_path);
    // printf("Cleaning up modules.\n");

    // // sample to get metadata and a specific field
    // cJSON *metadata = TCP_getMetadata();
    // cJSON *title = cJSON_GetObjectItem(metadata, "title");
    // printf("Title: %s\n", title ? title->valuestring : "N/A");

    // Let's do have the program run normally, if 


    printf("Program completely successfully.\n");
    return 0;
}