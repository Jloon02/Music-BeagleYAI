// Parts of code adapted from audio_cmake.zip provided by Dr. Brian
#include "hal/wavePlayback.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>


static snd_pcm_t *handle = NULL;
#define DEFAULT_VOLUME 80

#define DATA_OFFSET_INTO_WAVE 44
#define SAMPLE_RATE 88200
#define NUM_CHANNELS 1
#define SAMPLE_SIZE (sizeof(short)) // bytes per sample

#define SOURCE_FILE "wave-files/jaded.wav"


static int volume = 0;
static bool isInitialized = false;

//
static _Bool playing = false;
static pthread_t playbackThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;

// Playback threading
void* playbackThread2(void* arg);

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


snd_pcm_t *WavePlayback_openDevice(void)
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

// TODO: Since wave files can be very large, need to divide into chunks so the song can be played
// Read the entire wave file into memory.
// This is BLOCKING
void WavePlayback_readWaveFileIntoMemory(char *fileName, wavedata_t2 *pWaveStruct)
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

// This is BLOCKING
void WavePlayback_playFile(snd_pcm_t *handle, wavedata_t2 *pWaveData)
{
    // If anything is waiting to be written to screen, can be delayed unless flushed
    fflush(stdout);
    
    // Write the entire WAV data to the PCM device and wait until it is done.
    // `snd_pcm_writei` writes the data to the playback device. It blocks until the entire
    // data is sent to the hardware.
    snd_pcm_sframes_t frames = snd_pcm_writei(handle, pWaveData->pData, pWaveData->numSamples);

    // Check for errors
    if (frames < 0) {
        frames = snd_pcm_recover(handle, frames, 0);
    }
    if (frames < 0) {
        fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
        exit(EXIT_FAILURE);
    }
    if (frames > 0 && frames < pWaveData->numSamples) {
        printf("Short write (expected %d, wrote %li)\n", pWaveData->numSamples, frames);
    }
}

// Stream and play the file in chuncks
void WavePlayback_streamFile(snd_pcm_t *handle, char *fileName)
{
    // Open the file to read in binary
    FILE *file = fopen(fileName, "rb");
    if (!file) {
        fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
        exit(EXIT_FAILURE);
    }

    // Skip header information
    fseek(file, DATA_OFFSET_INTO_WAVE, SEEK_SET);

    // Define the buffer size, which is the number of samples to read at once.
    const int BUFFER_SIZE = 4096;   // Prev value: 4096
    short buffer[BUFFER_SIZE];

    size_t samplesRead;
    while ((samplesRead = fread(buffer, SAMPLE_SIZE, BUFFER_SIZE, file)) > 0) {
        pthread_mutex_lock(&audioMutex);
        float volumeFactor = volume / 100.0f; // convert volume to a scale 0 to 1
        pthread_mutex_unlock(&audioMutex);

        for (size_t i = 0; i < samplesRead; i++) {
            buffer[i] = (short)(buffer[i] * volumeFactor);
        }

        // Write the chunk of audio data to the PCM device.
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer, samplesRead);

        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0);
        }
        if (frames < 0) {
            fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_write(): %li\n", frames);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);
}

void WavePlayback_cleanAll(snd_pcm_t *handle, wavedata_t2 *pWaveData)
{
    snd_pcm_drain(handle);
    snd_pcm_hw_free(handle);
    snd_pcm_close(handle);
    free(pWaveData->pData);
}

// When called, this function starts the thread and plays music
void WavePlayback_startThread(void)
{
    if (!playing) {
        playing = true;
        pthread_create(&playbackThreadId, NULL, playbackThread2, NULL);
    }
}

void WavePlayback_stopPlayback(void)
{
    playing = false;
    pthread_join(playbackThreadId, NULL);
    
    // Drain and prepare the device for next use
    if (handle) {
        snd_pcm_drain(handle);
        snd_pcm_prepare(handle);
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

    pthread_join(playbackThreadId, NULL);

    WavePlayback_cleanAll(handle, NULL);
}

void* playbackThread2(void* _arg)
{
	(void)_arg;
    if (handle == NULL) {
        handle = WavePlayback_openDevice();  // Reopen the device if handle is null
    }

    while (playing) {
        wavedata_t2 sampleFile;
        WavePlayback_readWaveFileIntoMemory(SOURCE_FILE, &sampleFile);
        
        // Prepare the device before playing
        int err = snd_pcm_prepare(handle);
        if (err < 0) {
            printf("Prepare error: %s\n", snd_strerror(err));
            break;
        }
        
        WavePlayback_streamFile(handle, SOURCE_FILE);
        
        // Clean up the sample data but keep the device open
        free(sampleFile.pData);

        playing = false;
    }

	return NULL;
}