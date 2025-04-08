#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <string.h>
#include <signal.h>     //signal()
#include <stdbool.h>
#include <unistd.h>     //usleep()

#include "lcd_draw.h"
#include "hal/timeFunction.h"
#include "hal/wavePlayback.h"
#include "hal/rotary_encoder.h"
#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include "tcp_server.h"
#include "song_metadata.h"

#include <arpa/inet.h>
#include <cjson/cJSON.h>

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

// Song Information
static char* song_name      = "NA";
static char* artist_name    = "NA"; 
static char* album_name     = "NA";
static char* release_date   = "NA";
static char* spotify_url    = "NA";
static char* apple_url      = "NA"; 
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
    Paint_DrawString_EN(0, y, albumBuffer, &Font12, WHITE, BLACK);
    y += 30;

    // // Display Release Date
    char releaseBuffer[buffer_size];
    truncate_to_fit(releaseBuffer, sizeof(releaseBuffer), "Release:", release_date, 8, LCD_1IN54_WIDTH);
    Paint_DrawString_EN(0, y, releaseBuffer, &Font12, WHITE, BLACK);
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

// Function that updates the information, and tells if we should update screen
static bool retrieveUpdateMetadata(void)
{
    // Retrieve metadata from the server;
    cJSON *metadata = SongMetadata_getMetadata();
    if (metadata != NULL) {
        // Get the title from the metadata
        cJSON *title = cJSON_GetObjectItem(metadata, "title");
        cJSON *artist = cJSON_GetObjectItem(metadata, "artist");
        cJSON *album = cJSON_GetObjectItem(metadata, "album");
        cJSON *release = cJSON_GetObjectItem(metadata, "release_date");
        cJSON *spotify = cJSON_GetObjectItem(metadata, "spotify_url");
        cJSON *apple = cJSON_GetObjectItem(metadata, "apple_music_url");

        // Temporary variables with default values (not yet allocated)
        char *temp_song_name = NULL;
        char *temp_artist_name = NULL;
        char *temp_album_name = NULL;
        char *temp_release_date = NULL;
        char *temp_spotify_url = NULL;
        char *temp_apple_url = NULL;

        // Allocate and copy only if the field is a valid string
        if (cJSON_IsString(title) && title->valuestring != NULL) {
            temp_song_name = strdup(title->valuestring);
        } else {
            temp_song_name = strdup("N/A");
        }
        if (cJSON_IsString(artist) && artist->valuestring != NULL) {
            temp_artist_name = strdup(artist->valuestring);
        } else {
            temp_artist_name = strdup("N/A");
        }
        if (cJSON_IsString(album) && album->valuestring != NULL) {
            temp_album_name = strdup(album->valuestring);
        } else {
            temp_album_name = strdup("N/A");
        }
        if (cJSON_IsString(release) && release->valuestring != NULL) {
            temp_release_date = strdup(release->valuestring);
        } else {
            temp_release_date = strdup("N/A");
        }
        if (cJSON_IsString(spotify) && spotify->valuestring != NULL) {
            temp_spotify_url = strdup(spotify->valuestring);
        } else {
            temp_spotify_url = strdup("N/A");
        }
        if (cJSON_IsString(apple) && apple->valuestring != NULL) {
            temp_apple_url = strdup(apple->valuestring);
        } else {
            temp_apple_url = strdup("N/A");
        }

        // Compare the song, artist, album to see if we should update.
        if (strcmp(song_name, temp_song_name) != 0 || 
            strcmp(artist_name, temp_artist_name) != 0  ||
            strcmp(album_name, temp_album_name) != 0) {
            // Free the previous song_name memory if it was dynamically allocated
            if (song_name && song_name != "NA") {
                free(song_name);
            }
            if (artist_name && artist_name != "NA") {
                free(artist_name);
            }
            if (album_name && album_name != "NA") {
                free(album_name);
            }
            if (release_date && release_date != "NA") {
                free(release_date);
            }
            if (spotify_url && spotify_url != "NA") {
                free(spotify_url);
            }
            if (apple_url && apple_url != "NA") {
                free(apple_url);
            }

            // Update the song_name with the new value
            song_name = temp_song_name;
            artist_name = temp_artist_name;
            album_name = temp_album_name;
            release_date = temp_release_date;
            spotify_url = temp_spotify_url;
            apple_url = temp_apple_url;
            return true;
        } else {
            // If no change in song name, just free temp_song_name
            free(temp_song_name);
            free(temp_artist_name);
            free(temp_album_name);
            free(temp_release_date);
            free(temp_spotify_url);
            free(temp_apple_url);
        }

    }

    return false;

}

static void* lcdUpdateFunction(void* arg)
{
    (void) arg;
    Lcd_draw_songScreen();
    while (updateLCD) {

        int temp_volume = WavePlayback_getVolume();
        if (current_volume != temp_volume) {
            current_volume = temp_volume;
            lcd_draw_volume();
        }

        if (retrieveUpdateMetadata()) {
            Lcd_draw_songScreen();
        }
        sleep_for_ms(50);  // Add a small delay before checking again
        
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