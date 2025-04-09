// Parts of code adapted from audio_cmake.zip provided by Dr. Brian
#include "hal/wavePlayback.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

static snd_pcm_t *handle = NULL;
#define DEFAULT_VOLUME 80

#define DATA_OFFSET_INTO_WAVE 44
#define SAMPLE_RATE 88200
#define NUM_CHANNELS 1
#define SAMPLE_SIZE (sizeof(short)) // bytes per sample
#define JUMP_DURATION 10

static int volume = 0;
static float currentAmplitude = 0.0f;
static bool isInitialized = false;

static char* file_path = "";

//
static bool paused = false; 
static bool playing = false;
static pthread_t playbackThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;

// Audio file currently playing
static FILE *audioFile = NULL;
static long currentSampleOffset = 0;

// Playback threading
void* playbackThread(void* _arg);
int WavePlayback_getVolume()
{
	// Return the cached volume; good enough unless someone is changing
	// the volume through other means and the cached value is out of date.
	return volume;
}

void WavePlayback_setVolume(int newVolume)
{
	// Ensure volume is reasonable; If so, cache it for later getVolume() calls.
	if (newVolume < 0 || newVolume > AUDIOMIXER_MAX_VOLUME) {
		printf("ERROR: Volume must be between 0 and 100.\n");
		return;
	}
	volume = newVolume;
    printf("SET VOLUME TO: %d\n", newVolume);
    long min, max;
    snd_mixer_t *mixerHandle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "PCM";	// For ZEN cape
    //const char *selem_name = "Speaker";	// For USB Audio

    snd_mixer_open(&mixerHandle, 0);
    snd_mixer_attach(mixerHandle, card);
    snd_mixer_selem_register(mixerHandle, NULL, NULL);
    snd_mixer_load(mixerHandle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(mixerHandle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(mixerHandle);
}

// Return true if a song is currently playing
bool WavePlayback_isPlaying(void) {
    return playing;
}

// Return true if song is paused
bool WavePlayback_isPaused(void) {
    return paused;
}

static snd_pcm_t *WavePlayback_openDevice(void)
{
    if (handle != NULL) {
        return handle;
    }
    // Open PCM output
    int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        printf("Play-back open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    err = snd_pcm_set_params(handle, 
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        NUM_CHANNELS,
        SAMPLE_RATE,
        1,      // Allow software resampling
        50000); // 0.05 seconds per buffer
    
    if (err < 0) {
        printf("Play-back configuration error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    return handle;
}

static void WavePlayback_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct)
{
    assert(pWaveStruct);

    FILE *file = fopen(fileName, "rb");
    if (!file) {
        fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
        exit(EXIT_FAILURE);
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    int sizeInBytes = ftell(file) - DATA_OFFSET_INTO_WAVE;
    fseek(file, DATA_OFFSET_INTO_WAVE, SEEK_SET);
    pWaveStruct->numSamples = sizeInBytes / SAMPLE_SIZE;

    // Allocate memory
    pWaveStruct->pData = malloc(sizeInBytes);
    if (!pWaveStruct->pData) {
        fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n", sizeInBytes, fileName);
        exit(EXIT_FAILURE);
    }

    // Read data
    int samplesRead = fread(pWaveStruct->pData, SAMPLE_SIZE, pWaveStruct->numSamples, file);
    if (samplesRead != pWaveStruct->numSamples) {
        fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
                pWaveStruct->numSamples, fileName, samplesRead);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

void WavePlayback_jumpForward(void)
{
    if (!playing || !audioFile) return;

    pthread_mutex_lock(&audioMutex);
    
    // Calculate how many samples to skip (10 seconds worth)
    long samplesToSkip = JUMP_DURATION * SAMPLE_RATE;
    long newPosition = currentSampleOffset + samplesToSkip;
    
    // Get total samples in file
    fseek(audioFile, 0, SEEK_END);
    long totalSamples = (ftell(audioFile) - DATA_OFFSET_INTO_WAVE) / SAMPLE_SIZE;
    fseek(audioFile, currentSampleOffset * SAMPLE_SIZE + DATA_OFFSET_INTO_WAVE, SEEK_SET);
    
    // Don't go past end of file
    if (newPosition > totalSamples) {
        newPosition = totalSamples;
    }
    
    // Update position
    currentSampleOffset = newPosition;
    fseek(audioFile, currentSampleOffset * SAMPLE_SIZE + DATA_OFFSET_INTO_WAVE, SEEK_SET);
    
    // For ALSA, we need to clear the buffer to avoid playing old data
    snd_pcm_drop(handle);
    snd_pcm_prepare(handle);
    
    pthread_mutex_unlock(&audioMutex);
    
    printf("Jumped forward 10 seconds\n");
}

void WavePlayback_jumpBackward(void) {
    if (!playing || !audioFile) return;

    pthread_mutex_lock(&audioMutex);
    
    // Pause playback during seek
    bool wasPaused = paused;
    paused = true;
    
    // Calculate new position
    long samplesToRewind = JUMP_DURATION * SAMPLE_RATE;
    long newPosition = currentSampleOffset - samplesToRewind;
    newPosition = (newPosition < 0) ? 0 : newPosition;
    
    // Update position
    currentSampleOffset = newPosition;
    fseek(audioFile, currentSampleOffset * SAMPLE_SIZE + DATA_OFFSET_INTO_WAVE, SEEK_SET);
    
    // Reset ALSA
    snd_pcm_drop(handle);
    snd_pcm_prepare(handle);
    
    // Restore pause state
    paused = wasPaused;
    
    pthread_mutex_unlock(&audioMutex);
    
    printf("Jumped backward 10 seconds\n");
}

// Stream and play the file in chuncks
static void WavePlayback_streamFile(snd_pcm_t *handle, char *fileName)
{
    // Open the file to read in binary
    audioFile = fopen(fileName, "rb");
    if (!audioFile) {
        fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
        exit(EXIT_FAILURE);
    }

    // Skip header information and also current offset
    fseek(audioFile, currentSampleOffset * SAMPLE_SIZE + DATA_OFFSET_INTO_WAVE, SEEK_SET);

    // Define the buffer size, which is the number of samples to read at once.
    const int BUFFER_SIZE = 4096;   // Prev value: 4096
    short buffer[BUFFER_SIZE];
    size_t samplesRead;

    while ((samplesRead = fread(buffer, SAMPLE_SIZE, BUFFER_SIZE, audioFile)) > 0) {
        currentSampleOffset += samplesRead;
        if (paused) {
            snd_pcm_pause(handle, 1); // Pause ALSA playback
            while (paused && playing) {
                usleep(10000); // Sleep 10ms to avoid busy-waiting
            }
            snd_pcm_pause(handle, 0); // Resume ALSA playback
            if (!playing) break; // Exit if stopped while paused
        }
        if (!playing) break;
        
        pthread_mutex_lock(&audioMutex);
        float volumeFactor = volume / 100.0f; // convert volume to a scale 0 to 1
        pthread_mutex_unlock(&audioMutex);
        
        // Calculate amplitude for this buffer
        float maxAmplitude = 0.0f;
        for (size_t i = 0; i < samplesRead; i++) {
            buffer[i] = (short)(buffer[i] * volumeFactor);
            float sample = (float)buffer[i] / 32768.f; // Normalizing to [-1.0, 1.0]
            if (fabsf(sample) > maxAmplitude) {
                maxAmplitude = fabsf(sample);
            }
        }

        currentAmplitude = maxAmplitude;

        // Write the chunk of audio data to the PCM device.
        // Need thread or else pcm error will occur if spam-clicking the buttons
        pthread_mutex_lock(&audioMutex);
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer, samplesRead);
        pthread_mutex_unlock(&audioMutex);

        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0);
        }
        if (frames < 0) {
            fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_write(): %li\n", frames);
            exit(EXIT_FAILURE);
        }
    }

    fclose(audioFile);
    audioFile = NULL;
    currentSampleOffset = 0;
}

float WavePlayback_getCurrentAmplitude(void) {
    return currentAmplitude;
}

// When called, this function starts the thread and plays music
void WavePlayback_startThread(const char* path)
{
    if (!playing) {
        playing = true;
        file_path = strdup(path);
        pthread_create(&playbackThreadId, NULL, playbackThread, NULL);
    }
}

// Function to stop playback, might be needed later if we want user to be able to stop music, need to remove static for that
void WavePlayback_stopPlayback(void)
{
    if(playing){
        playing = false;
        paused = false;
        pthread_join(playbackThreadId, NULL);

        // Free the memory
        if (file_path && *file_path != '\0') {
            free(file_path);
            file_path = "";
        }
        
        // Drain and prepare the device for next use
        if (handle) {
            snd_pcm_drain(handle);
            snd_pcm_prepare(handle);
        }
        printf("done\n");
    }
}

void WavePlayback_init(void)
{
    assert(!isInitialized);
    WavePlayback_setVolume(DEFAULT_VOLUME);

    isInitialized = true;
}

void WavePlayback_cleanup(void)
{
    assert(isInitialized);
    isInitialized = false;
    playing = false;

    pthread_join(playbackThreadId, NULL);

    WavePlayback_stopPlayback();
}

void WavePlayback_pausePlayback(void)
{
    if (playing && !paused) {
        paused = true;
    }
}

void WavePlayback_resumePlayback(void)
{
    if (playing && paused) {
        paused = false;
    }
}

void* playbackThread(void* _arg)
{
	(void)_arg;
    if (handle == NULL) {
        handle = WavePlayback_openDevice();  // Reopen the device if handle is null
    }

    while (playing) {
        wavedata_t sampleFile;
        WavePlayback_readWaveFileIntoMemory(file_path, &sampleFile);
        
        // Prepare the device before playing
        int err = snd_pcm_prepare(handle);
        if (err < 0) {
            printf("Prepare error: %s\n", snd_strerror(err));
            break;
        }
        
        WavePlayback_streamFile(handle, file_path);
        
        // Clean up the sample data but keep the device open
        free(sampleFile.pData);

        playing = false;
    }

	return NULL;
}