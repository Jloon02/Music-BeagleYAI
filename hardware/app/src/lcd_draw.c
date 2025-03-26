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
// #include "hal/audioMixer.h"
#include "hal/wavePlayback.h"
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
static const int buffer_size = 48;
static const int url_buffer_size = 128;

static int current_screen = 0;

// static const int x = 5;
static const int volumeY = 220;

// Screen one
static int current_volume = 80;
// static int current_bpm = 120;

// Song Information
static char* song_name      = "NA"; // "IRIS ASLDKASLMDASOCMLJAOSKXLMAODMLASDJAPSMXOAMOMXMASPDKASDJMPOQJDPMSMXOAKSPXASDLJFAO@)EKFSSSSSFMASDLKFASLDFKOJFAKSWEFCMAS";
static char* artist_name    = "NA"; // "The Goo Goo Dolls"
static char* album_name     = "NA"; // "Dizzy up the Girl"
static char* release_date   = "NA"; // "1998"
static char* spotify_url    = "NA"; // "https://www.spotify.com/lmaoxd/ajsdhn108jlaksnDASD!9jnaxsou")
static char* apple_url      = "NA"; // "https://www.apple.com/music/kasANSD1VNK9j1lmsd"

static pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to help truncate strings that won't fit in the buffer
static void truncate_to_fit(char* dest, size_t dest_size, const char* label, const char* text, int font_width, int max_width_px) {
    int label_len = strlen(label);
    int max_chars_total = max_width_px / font_width;
    int max_text_chars = max_chars_total - label_len;

    if (max_text_chars <= 0) {
        // Not enough space to show even the label
        dest[0] = '\0';
        return;
    }

    int text_len = strlen(text);
    if (text_len > max_text_chars) {
        // Reserve 3 spaces for "..."
        int visible_chars = max_text_chars - 3;
        if (visible_chars < 0) visible_chars = 0; // just in case
        snprintf(dest, dest_size, "%s%.*s...", label, visible_chars, text);
    } else {
        snprintf(dest, dest_size, "%s%s", label, text);
    }
}

void Lcd_draw_songScreen(void)
{
    assert (isInitialized);
    Paint_Clear(WHITE);

    // TODO:
    // How to make sure the text fits on screen, also that it won't overflow?

    int y = 0;
    // Display Song name 
    char titleBuffer[buffer_size];
    truncate_to_fit(titleBuffer, sizeof(titleBuffer), "Song:", song_name, 8, LCD_1IN54_WIDTH);
    Paint_DrawString_EN(0, y, titleBuffer, &Font16, WHITE, BLACK);
    y += 40;

    // Display Artist
    char artistBuffer[buffer_size];
    truncate_to_fit(artistBuffer, sizeof(artistBuffer), "Artist:", artist_name, 8, LCD_1IN54_WIDTH);
    Paint_DrawString_EN(0, y, artistBuffer, &Font16, WHITE, BLACK);
    y += 40;

    // Display Album
    char albumBuffer[buffer_size];
    truncate_to_fit(albumBuffer, sizeof(albumBuffer), "Album:", album_name, 8, LCD_1IN54_WIDTH);
    Paint_DrawString_EN(0, y, albumBuffer, &Font16, WHITE, BLACK);
    y += 40;

    // // Display Release Date
    char releaseBuffer[buffer_size];
    truncate_to_fit(releaseBuffer, sizeof(releaseBuffer), "Release:", release_date, 8, LCD_1IN54_WIDTH);
    Paint_DrawString_EN(0, y, releaseBuffer, &Font16, WHITE, BLACK);
    y += 20;

    // // Display Spotify URL
    char spotifyBuffer[url_buffer_size];
    snprintf(spotifyBuffer, sizeof(spotifyBuffer), "Spotify URL:%s", spotify_url);
    Paint_DrawString_EN(0, y, spotifyBuffer, &Font12, WHITE, BLACK);
    y += 40;

    // Display Apple Music URL
    char appleBuffer[url_buffer_size];
    snprintf(appleBuffer, sizeof(appleBuffer), "Apple URL:%s", apple_url);
    Paint_DrawString_EN(0, y, appleBuffer, &Font12, WHITE, BLACK);
    y += 40;


    // // Display Volume
    char volumeBuffer[buffer_size];
    snprintf(volumeBuffer, sizeof(volumeBuffer), "Volume:%d ", current_volume);
    Paint_DrawString_EN(0, volumeY, volumeBuffer, &Font16, WHITE, BLACK);


    LCD_1IN54_DisplayWindows(0, 0, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, s_fb);
}

static void lcd_draw_volume(void)
{
    printf("Drawing Volum!\n");
    Paint_Clear(WHITE);
    char volumeBuffer[buffer_size];
    snprintf(volumeBuffer, sizeof(volumeBuffer), "Volume:%d ", current_volume);
    Paint_DrawString_EN(0, volumeY, volumeBuffer, &Font16, WHITE, BLACK);

    LCD_1IN54_DisplayWindows(0, volumeY, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, s_fb);

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
    Lcd_draw_songScreen();
    while (updateLCD) {
        // TODO
        // When we get new information from song, update the song screen

        int temp_volume = WavePlayback_getVolume();
        if (current_volume != temp_volume) {
            current_volume = temp_volume;
            lcd_draw_volume();
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