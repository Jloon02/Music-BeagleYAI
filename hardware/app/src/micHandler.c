#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// COMMAND
// arecord -D hw:CARD=Device,DEV=0 -f S16_LE -r 44100 --duration=10 test.wav

#define MIC_DEVICE "hw:CARD=Device,DEV=0"
#define MIC_FORMAT "S16_LE"
#define MIC_RATE "44100"
#define DIRECTORY "wave-files/"

int counter=0;

void micHandler_startRecording(int duration){
  pid_t pid = fork();
  if(pid < 0){
    perror("Error creating fork");
  }

  // Child process
  if(pid == 0){
    char filename[64];
    snprintf(filename, sizeof(filename), "%srecording_%d.wav", DIRECTORY, counter++);

    char duration_str[16];
    snprintf(duration_str, sizeof(duration_str), "%d", duration);

    char *args[] = {
      "arecord",
      "-D", MIC_DEVICE,
      "-f", MIC_FORMAT,
      "-r", MIC_RATE,
      "--duration", duration_str,
      filename,
      NULL //needs to be null terminated
    };
    execvp("arecord", args);
    perror("Error when executing arecord");
    exit(EXIT_FAILURE);
  }
  else{
    printf("Recording in progress...\n");
    wait(NULL);  // Wait for child process to finish
    printf("Recording complete.\n");
  }
}