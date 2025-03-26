#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
// #include <gcrypt.h>

#define SERVER_IP "192.168.6.1"  // Server IP address
#define SERVER_PORT 12345      // Server port
#define BUFFER_SIZE 4096       // Buffer size for sending data
#define METADATA_END "<END_OF_METADATA>"
#define METADATA_END_LEN 17
#define FILE_END "<END>"

char metadata[BUFFER_SIZE];
size_t metadata_size = 0;

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

void receive_file(int client_socket, const char *output_file) {
    char buffer[BUFFER_SIZE + 1]; // +1 for null-termination
    FILE *file = NULL;
    int metadata_received = 0;
    size_t bytes_received;
    size_t metadata_end_pos = 0;
    size_t total_bytes_received = 0;
    
    // Reset global metadata for each new file
    metadata[0] = '\0';
    metadata_size = 0;

    // Open the output file for writing
    file = fopen(output_file, "wb");
    if (!file) {
        perror("Failed to open output file");
        return;
    }
    
    // receive metadata until we find the end marker
    while (!metadata_received) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            perror("Error receiving data");
            return;
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
            size_t file_data_start = metadata_end_pos + METADATA_END_LEN;
            size_t remaining_bytes = bytes_received - file_data_start;
            
            if (remaining_bytes > 0) {
                fwrite(buffer + file_data_start, 1, remaining_bytes, file);
                total_bytes_received += remaining_bytes;
            }
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
        fclose(file);
    }
}

void TCP_send_file_to_server(const char* file_path){
    // initialize_libgcrypt();

    // Create a socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Define server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    // printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // Send the WAV file
    send_file(client_socket, file_path);

    // Receive and parse metadata - in json
    // receive_metadata(client_socket);

    // Receive and save the processed file
    const char *output_file = "wave-files/processed_audio.wav";
    receive_file(client_socket, output_file);

    // Close the socket
    close(client_socket);
    printf("Connection closed.\n");
}