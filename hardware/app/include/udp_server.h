// udp_server.h
// Listens for incoming UDP packets using a thread
// Provides options to change mode, plays sounds, tempo, volume and shutdown

#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include <stdbool.h>

// Start the UDP server in its own thread
void UdpServer_start(void);

// Stop the UDP server and clean up resources.
void UdpServer_stop(void);

// Checks if UDP server is online or offline
bool UdpServer_isOnline(void);

#endif