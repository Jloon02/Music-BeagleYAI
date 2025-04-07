#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#include "cJSON.h"

static cJSON *metadata_array = 	NULL;
static cJSON *current_metadata = NULL;
static int metadata_array_size=0;
static int metadata_index = 0;

void loadMetadata();

void SongMetadata_readMetadataFile(char* metadata_file_name){
    if (metadata_array) {
        cJSON_Delete(metadata_array);
        metadata_array = NULL;
    }

    FILE *file = fopen(metadata_file_name, "r");
    if (!file) {
        printf("Failed to open metadata file");
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    // Allocate buffer for file contents
    char *file_contents = (char *)malloc(filesize + 1);
    if (!file_contents) {
        perror("Memory allocation failed");
        fclose(file);
        return;
    }

    // Read file into buffer
    fread(file_contents, 1, filesize, file);
    file_contents[filesize] = '\0';  // Null-terminate

    // Parse as JSON array
    metadata_array = cJSON_Parse(file_contents);
    if (!metadata_array || !cJSON_IsArray(metadata_array)) {

        printf("Error parsing JSON or root is not an array: %s\n", cJSON_GetErrorPtr());
        cJSON_Delete(metadata_array);  // Clean up just in case
        metadata_array = NULL;
    }
    metadata_array_size = cJSON_GetArraySize(metadata_array);
	loadMetadata();

    free(file_contents);
    fclose(file);
}

void SongMetadata_nextSong(){
	if(metadata_index < metadata_array_size){
		metadata_index++;
		loadMetadata();
	}
}

void SongMetadata_previousSong(){
	if(metadata_index > 0){
		metadata_index--;
		loadMetadata();
	}
}

cJSON* SongMetadata_getMetadata(){
	return current_metadata;
}

void loadMetadata(){
    if (metadata_array) {
		current_metadata = cJSON_GetArrayItem(metadata_array, metadata_index);
		// cJSON *title = cJSON_GetObjectItem(current_metadata, "title");
		// if (cJSON_IsString(title)) {
		// 	printf("Song Title: %s\n", title->valuestring);
		// }
    }
}