// udp_server.h
// Listens for incoming UDP packets using a thread
// Provides options to change mode, plays sounds, tempo, volume and shutdown

#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <stdbool.h>

// TCP only called when needed
void TCP_send_file_to_server(const char* file_path);

#endif