#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <gcrypt.h>

#define SERVER_IP "192.168.6.1"  // Server IP address
#define SERVER_PORT 12345      // Server port
#define BUFFER_SIZE 4096       // Buffer size for sending data
#define METADATA_END "<END_OF_METADATA>"
#define FILE_END "<END>"

void initialize_libgcrypt() {
    if (!gcry_check_version(GCRYPT_VERSION)) {
        fprintf(stderr, "libgcrypt version mismatch\n");
        exit(EXIT_FAILURE);
    }
    gcry_control(GCRYCTL_SET_THREAD_CBS, NULL); // optional, configure multi-threading if needed
}

// Function to calculate the MD5 checksum of a file
void calculate_md5(const char *file_path, unsigned char *result) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Failed to open file for MD5 calculation");
        return;
    }

    gcry_md_hd_t md5_handle;
    gcry_md_open(&md5_handle, GCRY_MD_MD5, 0);  // Open MD5 context

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        gcry_md_write(md5_handle, buffer, bytes_read);  // Update MD5 hash
    }
    fclose(file);

    // Finalize and get the hash result
    memcpy(result, gcry_md_read(md5_handle, 0), gcry_md_get_algo_dlen(GCRY_MD_MD5));
    gcry_md_close(md5_handle);;
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


// Function to receive and save the processed file from the server
void receive_file(int client_socket, const char *output_file) {
    FILE *file = fopen(output_file, "wb");
    if (!file) {
        perror("Failed to create output file");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    // Ensure file is completely written
    if (bytes_received < 0) {
        perror("Error receiving file data");
    }

    fclose(file);
    printf("Processed file received and saved as: %s\n", output_file);
}

void receive_metadata(int client_socket) {
    char metadata[BUFFER_SIZE * 2] = {0};  // Buffer to accumulate metadata
    ssize_t bytes_received;
    size_t metadata_len = 0;

    while ((bytes_received = recv(client_socket, metadata + metadata_len, sizeof(metadata) - metadata_len - 1, 0)) > 0) {
        metadata_len += bytes_received;
        metadata[metadata_len] = '\0';  // Null terminate the string
        
        if (strstr(metadata, METADATA_END)) {
            break;  // Break the loop once we find the end marker
        }
    }
    printf("%s\n", metadata);
}

void TCP_send_file_to_server(const char* file_path){
    initialize_libgcrypt();

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
    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // Send the WAV file
    send_file(client_socket, file_path);

    // Receive and parse metadata
    receive_metadata(client_socket);

    // Receive and save the processed file
    const char *output_file = "wave-files/processed_audio.wav";
    receive_file(client_socket, output_file);

    // Calculate and display MD5 checksum
    unsigned char md5_result[16];  // MD5 produces a 16-byte hash
    calculate_md5(output_file, md5_result);
    printf("\nMD5 Checksum of received file: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", md5_result[i]);
    }
    printf("\n");

    // Close the socket
    close(client_socket);
    printf("Connection closed.\n");
}