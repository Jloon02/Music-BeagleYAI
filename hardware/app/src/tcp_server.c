#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"  // Server IP address
#define SERVER_PORT 12345      // Server port
#define BUFFER_SIZE 4096       // Buffer size for sending data

void TCP_client() {
const char *file_path = "../../wave-files/test.wav";  // Path to the .wav file to send
    FILE *file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

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

    // Open the .wav file
    file = fopen(file_path, "rb");
    if (!file) {
        perror("Failed to open file");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Send the file data
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) == -1) {
            perror("Failed to send file data");
            fclose(file);
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }
    printf("File sent: %s\n", file_path);

    // Close the file
    fclose(file);

    // Shutdown the write side of the socket to signal the server that we're done sending data
    shutdown(client_socket, SHUT_WR);

    // Receive a response from the server
    char response[1024];
    ssize_t response_len = recv(client_socket, response, sizeof(response) - 1, 0);
    if (response_len == -1) {
        perror("Failed to receive response");
    } else {
        response[response_len] = '\0';  // Null-terminate the response
        printf("Server response: %s\n", response);
    }

    // Close the socket
    close(client_socket);
}