// Manages the shutdown of the system
#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include "stdbool.h"

// Initialize shutdown module
void Shutdown_init(void);

// Cleanup shutdown module
void Shutdown_cleanup(void);

// Request program shutdown
void Shutdown_requestShutdown(void);

// Check if shutdown has been requested
bool Shutdown_isShutdownRequested(void);

#endif // SHUTDOWN_H