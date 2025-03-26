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

int main()
{
    printf("Starting Program.\n");

    // Initialize all modules; HAL modules first
    // WavePlayback_init();
    // Lcd_draw_init();
    // Gpio_initialize();
    // Rotary_encoder_init();
    // Joystick_init();
    // JoystickFunction_init();
    // RotaryEncoderFunction_init();
    
    // sleep(10);  // Sleep for 10 seconds

    // RotaryEncoderFunction_cleanup();
    // JoystickFunction_cleanup();
    // Joystick_cleanup();
    // Rotary_encoder_cleanup();
    // Gpio_cleanup();
    // Lcd_draw_cleanup();
    // WavePlayback_cleanup();
    
    
    // start mic then send
    micHandler_startRecording(5);
    printf("main has finished recording\n");

    char* file_path = micHandler_getRecordingPath();
    TCP_send_file_to_server(file_path);
    free(file_path);
    printf("Cleaning up modules.\n");


    // Let's do have the program run normally, if 


    printf("Program completely successfully.\n");
    return 0;
}