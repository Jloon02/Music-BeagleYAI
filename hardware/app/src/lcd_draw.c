#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()
#include <stdbool.h>
#include <unistd.h>     //usleep()

#include "lcd_draw.h"
#include "timeFunction.h"
#include "hal/audioMixer.h"
#include "beatboxGenerator.h"
#include "hal/periodTimer.h"
#include "hal/rotary_encoder.h"
#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"

#define THRESHOLD 0.0001

static UWORD *s_fb;
static pthread_t lcdUpdateThread;
static bool updateLCD = true;
static bool isInitialized = false;
static const int buffer_size = 24;

static int current_screen = 0;
static int prev_screen = 0;
// Screen one
static int current_volume = 80;
static int current_bpm = 120;
static BeatMode current_mode = BEAT_ROCK;
static pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;

void lcd_draw_screenOne(void)
{
    assert(isInitialized);
    const int x = 5;
    const int y = 70;
    // Change the beat name
    Paint_Clear(WHITE);
    char titleBuffer[buffer_size];
    snprintf(titleBuffer, sizeof(titleBuffer), "%s", BeatMode_toString(current_mode));
    int textWidth = strlen(titleBuffer) * 8; // Font16 width ~8px per char
    int centerX = (LCD_1IN54_WIDTH - textWidth) / 2;
    Paint_DrawString_EN(centerX, y, titleBuffer, &Font16, WHITE, BLACK);

    // Draw Volume in bottom-left
    char volumeBuffer[buffer_size];
    snprintf(volumeBuffer, sizeof(volumeBuffer), "Vol: %d", current_volume);
    int volY = LCD_1IN54_HEIGHT - 20;  // Bottom-left corner
    Paint_DrawString_EN(x, volY, volumeBuffer, &Font16, WHITE, BLACK);

    // Draw BPM in bottom-right
    char bpmBuffer[buffer_size];
    snprintf(bpmBuffer, sizeof(bpmBuffer), "BPM: %d", current_bpm); // Example BPM value
    int bpmTextWidth = strlen(bpmBuffer) * 8; // Font16 width ~8px per char
    int bpmX = LCD_1IN54_WIDTH - bpmTextWidth - 25; // Ensure there is enough space
    int bpmY = LCD_1IN54_HEIGHT - 20; 
    Paint_DrawString_EN(bpmX, bpmY, bpmBuffer, &Font16, WHITE, BLACK);

    LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, s_fb);
}

void lcd_draw_screenTwo(void)
{
    assert(isInitialized);

    const int x = 5;
    const int y = 70;
    int nextY = y;

    Period_statistics_t stats;
    Period_getStatisticsAndClear(PERIOD_EVENT_AUDIO_BUFFER, &stats);

    Paint_Clear(WHITE);
    char titleBuffer[buffer_size];
    snprintf(titleBuffer, sizeof(titleBuffer), "%s", "Audio Timing");
    int textWidth = strlen(titleBuffer) * 8; // Font16 width ~8px per char
    int centerX = (LCD_1IN54_WIDTH - textWidth) / 2;
    Paint_DrawString_EN(centerX, nextY, titleBuffer, &Font16, WHITE, BLACK);
    nextY += 20;

    char minBuffer[buffer_size];
    snprintf(minBuffer, sizeof(minBuffer), "min ms: %.3f", stats.minPeriodInMs);
    Paint_DrawString_EN(centerX, nextY, minBuffer, &Font16, WHITE, BLACK);
    nextY += 20;

    char maxBuffer[buffer_size];
    snprintf(maxBuffer, sizeof(maxBuffer), "max ms: %.3f", stats.maxPeriodInMs);
    Paint_DrawString_EN(centerX, nextY, maxBuffer, &Font16, WHITE, BLACK);
    nextY += 20;

    char avgBuffer[buffer_size];
    snprintf(avgBuffer, sizeof(avgBuffer), "avg ms: %.3f", stats.avgPeriodInMs);
    Paint_DrawString_EN(centerX, nextY, avgBuffer, &Font16, WHITE, BLACK);


    LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, s_fb);
}

void lcd_draw_screenThree(void)
{
    assert(isInitialized);

    const int x = 5;
    const int y = 70;
    int nextY = y;

    Period_statistics_t accelStats;
    Period_getStatisticsAndClear(PERIOD_EVENT_ACCELEORMETER, &accelStats);

    Paint_Clear(WHITE);
    char titleBuffer[buffer_size];
    snprintf(titleBuffer, sizeof(titleBuffer), "%s", "Accel. Timing");
    int textWidth = strlen(titleBuffer) * 8; // Font16 width ~8px per char
    int centerX = (LCD_1IN54_WIDTH - textWidth) / 2;
    Paint_DrawString_EN(centerX, nextY, titleBuffer, &Font16, WHITE, BLACK);
    nextY += 20;

    char minBuffer[buffer_size];
    snprintf(minBuffer, sizeof(minBuffer), "min ms: %.3f", accelStats.minPeriodInMs);
    Paint_DrawString_EN(centerX, nextY, minBuffer, &Font16, WHITE, BLACK);
    nextY += 20;

    char maxBuffer[buffer_size];
    snprintf(maxBuffer, sizeof(maxBuffer), "max ms: %.3f", accelStats.maxPeriodInMs);
    Paint_DrawString_EN(centerX, nextY, maxBuffer, &Font16, WHITE, BLACK);
    nextY += 20;

    char avgBuffer[buffer_size];
    snprintf(avgBuffer, sizeof(avgBuffer), "avg ms: %.3f", accelStats.avgPeriodInMs);
    Paint_DrawString_EN(centerX, nextY, avgBuffer, &Font16, WHITE, BLACK);

    LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, s_fb);
}

int Lcd_get_screen(void)
{
    pthread_mutex_lock(&screen_mutex);
    int screen = current_screen;
    pthread_mutex_unlock(&screen_mutex);
    return screen;
}

void Lcd_set_screen(void)
{
    pthread_mutex_lock(&screen_mutex);
    if (current_screen == 2) {
        current_screen = 0;
    } else {
        current_screen++;
    }
    pthread_mutex_unlock(&screen_mutex);
}

static void* lcdUpdateFunction(void* arg)
{
    (void) arg;
    while (updateLCD) {
        // If current and prev are different, we've changed screens
        int temp_screen = Lcd_get_screen();
        if (temp_screen != prev_screen) {
            Paint_Clear(WHITE);
            // Change screen
            if (temp_screen == 0) {
                lcd_draw_screenOne();
            }
            else if (temp_screen == 1)
            {
                Paint_Clear(WHITE);
                lcd_draw_screenTwo();
            } 
            else if (temp_screen == 2)
            {
                Paint_Clear(WHITE);
                lcd_draw_screenThree();
            }
            prev_screen = temp_screen;
        }

        switch (temp_screen) {
            case 0: {
                // Add code to handle screen 0
                int temp_bpm = BeatGenerator_getTempo();
                int temp_volume = AudioMixer_getVolume();
                BeatMode temp_mode = BeatGenerator_getMode();
                if (current_bpm != temp_bpm || current_volume != temp_volume || current_mode != temp_mode) {
                    current_bpm = temp_bpm;
                    current_volume = temp_volume;
                    current_mode = temp_mode;
                    lcd_draw_screenOne();
                }
            }
                break;
            case 1:
                // Add code to handle screen 1
                lcd_draw_screenTwo();
                break;
            case 2:
                // Add code to handle screen 2
                lcd_draw_screenThree();
                break;
            default:
                printf("Invalid screen\n");
                // Optional: Add code to handle invalid values (if necessary)
                break;
        }
        sleep_for_ms(50);
        
    }
    return NULL;
}

void Lcd_draw_init()
{
    assert(!isInitialized);
    
    // Module Init
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }
	
    // LCD Init
    DEV_Delay_ms(2000);
	LCD_1IN54_Init(HORIZONTAL);
	LCD_1IN54_Clear(WHITE);
	LCD_SetBacklight(1023);

    UDOUBLE Imagesize = LCD_1IN54_HEIGHT*LCD_1IN54_WIDTH*2;
    if((s_fb = (UWORD *)malloc(Imagesize)) == NULL) {
        perror("Failed to apply for black memory");
        exit(0);
    }
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    pthread_mutex_init(&screen_mutex, NULL);
    pthread_create(&lcdUpdateThread, NULL, lcdUpdateFunction, NULL);
    isInitialized = true;


}

void Lcd_draw_cleanup()
{
    assert(isInitialized);
    updateLCD = false;
    // pthread_cancel(lcdUpdateThread);
    pthread_join(lcdUpdateThread, NULL);
    pthread_mutex_destroy(&screen_mutex);
    // Module Exit
    if (s_fb != NULL) {
        free(s_fb);
        s_fb = NULL;
    }
	DEV_ModuleExit();
    isInitialized = false;
}

// void lcd_draw_updateScreen(char* message)
// {
//     assert(isInitialized);

//     const int x = 5;
//     const int y = 70;

//     // Initialize the RAM frame buffer to be blank (white)
//     Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
//     Paint_Clear(WHITE);

//     // Draw into the RAM frame buffer
//     // WARNING: Don't print strings with `\n`; will crash!
//     Paint_DrawString_EN(x, y, message, &Font16, WHITE, BLACK);

//     // Send the RAM frame buffer to the LCD (actually display it)
//     // Option 1) Full screen refresh (~1 update / second)
//     // LCD_1IN54_Display(s_fb);
//     // Option 2) Update just a small window (~15 updates / second)
//     //           Assume font height <= 20
//     LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, y+20, s_fb);
// }