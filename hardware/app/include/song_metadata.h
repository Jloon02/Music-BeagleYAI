// r5_led.h
// provides led signal to R5 based on amplitude of song
// communicate with the R5 through shared data

#include <cJSON.h>

#ifndef SONG_METADATA_H
#define SONG_METADATA_H

void SongMetadata_readMetadataFile(char* metadata_file_name);
void SongMetadata_nextSong();
void SongMetadata_previousSong();
cJSON* SongMetadata_getMetadata();

#endif