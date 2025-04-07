#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>

#include "song_metadata.h"
#include "hal/wavePlayback.h"

#define SERVER_IP "192.168.6.1" // "192.168.145.1" // "192.168.58.1" // "192.168.6.1"  // Server IP address
#define SERVER_PORT 12345      // Server port
#define BUFFER_SIZE 4096       // Buffer size for sending data
#define METADATA_END "<END_OF_METADATA>"
#define METADATA_END_LEN 17
#define FILE_END "<END>"
#define METADATA_FILE "MusicBoard-audio-files/saved_metadata"

char metadata[BUFFER_SIZE];
cJSON *metadata_json = NULL;
size_t metadata_size = 0;
char *output_file = NULL;

void parse_json(const char *json_str){
    // get metadata from json_str
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return;
    }
    metadata_json = root;

    // Update output file to "title-artist"
    cJSON *title = cJSON_GetObjectItem(root, "title");
    cJSON *artist = cJSON_GetObjectItem(root, "artist");
    if (title && artist && cJSON_IsString(title)) {
        if (output_file) {
            free(output_file);
        }

        output_file = malloc(BUFFER_SIZE);
        if (output_file == NULL) {
            perror("Failed to allocate memory for output_file");
            return;
        }

        snprintf(output_file, BUFFER_SIZE, "MusicBoard-audio-files/%s-%s.wav", 
            title->valuestring,
            artist->valuestring
        );
    } else {
        printf("No \"title\" field found in metadata\n");
    }
    printf("%s\n", output_file);

    FILE *file = fopen(METADATA_FILE, "r");
    cJSON *json_array = NULL;

    if (file) {
        // File exists, read its contents
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        rewind(file);

        char *file_content = malloc(length + 1);
        if (file_content) {
            fread(file_content, 1, length, file);
            file_content[length] = '\0';

            cJSON *existing = cJSON_Parse(file_content);
            if (existing && cJSON_IsArray(existing)) {
                json_array = existing;
            } else {
                json_array = cJSON_CreateArray();
                if (existing) cJSON_Delete(existing);
            }
            free(file_content);
        }
        fclose(file);
    } else {
        // File doesn't exist yet
        json_array = cJSON_CreateArray();
    }

    // Add the new metadata to the array
    cJSON_AddItemToArray(json_array, metadata_json);
    metadata_json = NULL; // Avoid double free later

    // Write the updated array back to the file
    char *json_string = cJSON_Print(json_array);
    file = fopen(METADATA_FILE, "w");
    if (file && json_string) {
        fwrite(json_string, 1, strlen(json_string), file);
        fclose(file);
    }

    // Cleanup
    free(json_string);
    cJSON_Delete(json_array);
}

// Function to send a file to the server
void send_file(int client_socket, const char *file_path) {
    printf("sending file %s\n", file_path);
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    // Send the file data
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == -1) {
            perror("Failed to send file data");
            fclose(file);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);

    // Signal the server that file transmission is done
    send(client_socket, FILE_END, strlen(FILE_END), 0);
    printf("File sent successfully: %s\n", file_path);
}

bool receive_file(int client_socket) {
    char buffer[BUFFER_SIZE + 1]; // +1 for null-termination
    FILE *file = NULL;
    int metadata_received = 0;
    size_t bytes_received;
    size_t metadata_end_pos = 0;
    size_t total_bytes_received = 0;
    size_t file_data_start = 0;
    size_t remaining_bytes = 0;
    
    // Reset global metadata for each new file
    metadata[0] = '\0';
    metadata_size = 0;
    
    // receive metadata until we find the end marker
    while (!metadata_received) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("No match found\n");
            return false;
        }

        // Ensure the buffer is null-terminated for strstr
        buffer[bytes_received] = '\0';

        // Check for end marker in the received chunk
        char *end_marker = strstr(buffer, METADATA_END);
        if (end_marker) {
            metadata_end_pos = end_marker - buffer;
            
            // Append the part before the marker to metadata
            memcpy(metadata + metadata_size, buffer, metadata_end_pos);
            metadata_size += metadata_end_pos;
            metadata[metadata_size] = '\0';
            
            metadata_received = 1;
            
            // Write the remaining data after the marker to the file
            file_data_start = metadata_end_pos + METADATA_END_LEN;
            remaining_bytes = bytes_received - file_data_start;
        } 
        // No end marker yet, append entire chunk to metadata
        else {
            memcpy(metadata + metadata_size, buffer, bytes_received);
            metadata_size += bytes_received;
            metadata[metadata_size] = '\0';
        }
    }

    // Print the received metadata
    printf("Received metadata:\n%s\n", metadata);
    parse_json(metadata);

    // Open the output file for writing
    file = fopen(output_file, "wb");
    if (!file) {
        perror("Failed to open output file");
        return false;
    }

    // write remaining bytes into file
    if (remaining_bytes > 0) {
        fwrite(buffer + file_data_start, 1, remaining_bytes, file);
        total_bytes_received += remaining_bytes;
    }

    // Continue receiving file data
    while (1) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        fwrite(buffer, 1, bytes_received, file);
        total_bytes_received += bytes_received;
    }

    if (file) {
        printf("File saved as:\n%s\n", output_file);
        fclose(file);
    }

    return true;
}

void TCP_sendFileToServer(const char* file_path){
    // initialize_libgcrypt();

    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        return;
    }

    // Define server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(client_socket);
        return;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(client_socket);
        return;
    }
    // printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // Send the WAV file
    send_file(client_socket, file_path);

    // Receive and parse metadata - in json
    // receive_metadata(client_socket);

    // Receive and save the processed file
    if(receive_file(client_socket)){
        WavePlayback_startThread(output_file);
    }

   SongMetadata_readMetadataFile(METADATA_FILE);

    // Close the socket
    close(client_socket);
    printf("Connection closed.\n");
}

cJSON* TCP_getMetadata() {
    return metadata_json;
}
