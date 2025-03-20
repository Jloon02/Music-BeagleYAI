// udp_server.c

#include "udp_server.h"
#include "hal/audioMixer.h"
#include "beatboxGenerator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>


// Constants
#define UDP_PORT 12345
#define BUFFER_SIZE 1024

// Global variables
static pthread_t udpThread;
static bool isInitialized = false;
static bool isUdpRunning = false;
static int sockfd;

bool UdpServer_isOnline(void) {
    return isUdpRunning;
}

// Function to handle UDP commands
static void handleCommand(int sockfd, struct sockaddr_in *clientAddr, socklen_t addrLen, const char *command) {
    char response[BUFFER_SIZE];
    int option;

    // Separate the option from command
    int argNum = sscanf(command, "%s %d", response, &option);

    if (argNum < 1) {
        printf("Invalid command format.\n");
        return;
    }

    if (strcmp(response, "mode") == 0) {
        // Handle mode command
        if (argNum == 2) {
            // Change drum-beat mode if option is provided
            if (option >= BEAT_NONE && option <= BEAT_CUSTOM) {
                BeatGenerator_setMode((BeatMode)option);
                snprintf(response, BUFFER_SIZE, "Mode set to %d\n", option);
            } else {
                snprintf(response, BUFFER_SIZE, "Invalid mode: %d\n", option);
            }
        } else {
            // Return current mode if no option is provided
            BeatMode currentMode = BeatGenerator_getMode();
            snprintf(response, BUFFER_SIZE, "%d", currentMode);
        }
    } else if (strcmp(response, "volume") == 0) {
        // Handle volume command
        if (argNum == 2) {
            // Adjust volume if option is provided
            if (option >= 0 && option <= 100) {
                AudioMixer_setVolume(option);
                snprintf(response, BUFFER_SIZE, "Volume set to %d\n", option);
            } else {
                snprintf(response, BUFFER_SIZE, "Invalid volume: %d\n", option);
            }
        } else {
            // Return current volume if no option is provided
            int currentVolume = AudioMixer_getVolume();
            snprintf(response, BUFFER_SIZE, "%d", currentVolume);
        }
    } else if (strcmp(response, "tempo") == 0) {
        // Handle tempo command
        if (argNum == 2) {
            // Adjust tempo if option is provided
            if (option >= MIN_TEMPO && option <= MAX_TEMPO) {
                BeatGenerator_setTempo(option);
                snprintf(response, BUFFER_SIZE, "Tempo set to %d BPM\n", option);
            } else {
                snprintf(response, BUFFER_SIZE, "Invalid tempo: %d\n", option);
            }
        } else {
            // Return current tempo if no option is provided
            int currentTempo = BeatGenerator_getTempo();
            snprintf(response, BUFFER_SIZE, "%d", currentTempo);
        }
    } else if (strcmp(response, "play") == 0) {
        // Handle play command
        if (argNum == 2) {
            // Play a specific sound if option is provided
            if (option == 0) {
                BeatGenerator_playDrum();
                snprintf(response, BUFFER_SIZE, "Playing base drum\n");
            } else if (option == 1) {
                BeatGenerator_playHat();
                snprintf(response, BUFFER_SIZE, "Playing hi-hat\n");
            } else if (option == 2) {
                BeatGenerator_playSnare();
                snprintf(response, BUFFER_SIZE, "Playing snare\n");
            } else {
                snprintf(response, BUFFER_SIZE, "Invalid sound\n");
            }
        } else {
            // Return error if no option is provided
            snprintf(response, BUFFER_SIZE, "Usage: play <sound> (0: DRUM, 1: HI_HAT, 2: SNARE)\n");
        }
    } else if (strcmp(response, "stop") == 0) {
        // Shut down the application
        snprintf(response, BUFFER_SIZE, "Shutting down...\n");
        isUdpRunning = false;
    } else if (strcmp(response, "?") == 0 || strcmp(response, "help") == 0) {
        // Display help message
        snprintf(response, BUFFER_SIZE,
                 "Accepted command examples:\n"
                 "mode <mode> -- Change drum-beat mode (0: None, 1: Rock, 2: Custom).\n"
                 "volume <volume> -- Set volume (0-100).\n"
                 "tempo <tempo> -- Set tempo (40-300 BPM).\n"
                 "play <sound> -- Play a sound (0: DRUM, 1: HI_HAT, 2: SNARE).\n"
                 "stop -- Shut down the application.\n"
                 "<enter> -- Repeat last command.\n");
    } else {
        snprintf(response, BUFFER_SIZE, "Unknown command. Type 'help' or '?' for a command list.\n");
    }

    // Send response to the client
    sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)clientAddr, addrLen);
}

// Thread to run the UDP server
static void* udpThreadFunc(void* none) {
    (void)none; // Suppress unused parameter warning

    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];

    // Socket creation UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP_server: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    // https://opencoursehub.cs.sfu.ca/bfraser/grav-cms/cmpt433/notes/files/06-LinuxProgramming-c.pdf
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(UDP_PORT);

    // Setting server address to bind the socket
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP server started on port %d\n", UDP_PORT);

    // Main server loop
    while (isUdpRunning) {
        // Receive a command from the client
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addrLen);
        buffer[n] = '\0';  // Null-terminate the received data

        // Handle the command
        handleCommand(sockfd, &clientAddr, addrLen, buffer);
    }

    // Finish
    printf("UDP server stopped\n");
    return NULL;
}

// Start the UDP server
void UdpServer_start(void) {
    assert(!isInitialized);

    isUdpRunning = true;
    isInitialized = true;

    // Start UDP server thread
    if (pthread_create(&udpThread, NULL, udpThreadFunc, NULL) != 0) {
        perror("Error when attempting to start up UDP server thread");
        exit(EXIT_FAILURE);
    }
}

// Stop the UDP server
void UdpServer_stop(void) {
    assert(isInitialized);

    isUdpRunning = false;
    isInitialized = false;

    // Clean up thread
    close(sockfd);
    pthread_cancel(udpThread);
    pthread_join(udpThread, NULL);
}