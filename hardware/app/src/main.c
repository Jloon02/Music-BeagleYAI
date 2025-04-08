// Main program to build the application
// Has main(); does initialization and cleanup and perhaps some basic logic.

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "shutdown.h"

int main()
{
    printf("Starting Program.\n");

    // Initialize all modules; HAL modules first
    Shutdown_init();
    while (!Shutdown_isShutdownRequested()) {

    }
    Shutdown_cleanup();

    printf("Program completely successfully.\n");
    return 0;
}