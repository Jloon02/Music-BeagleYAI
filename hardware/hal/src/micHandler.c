#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// COMMAND
// arecord -D hw:CARD=Device,DEV=0 -f S16_LE -r 44100 --duration=10 test.wav

#define MIC_DEVICE "hw:CARD=SoloCast,DEV=0"
#define MIC_FORMAT "S16_LE"
#define MIC_RATE "44100"
#define DIRECTORY "MusicBoard-audio-files/"

char* micHandler_getRecordingPath(){
  char *filename = malloc(64);
  snprintf(filename, 64, "%sBeagle_recording.wav", DIRECTORY);
  return filename;
}

void micHandler_startRecording(int duration){
  pid_t pid = fork();
  if(pid < 0){
    perror("Error creating fork");
  }

  // Parent
  if(pid > 0){
    // counter++;  // Increment the global counter in the parent
    printf("Recording in progress...\n");
    wait(NULL);  // Wait for child process to finish
    printf("Recording complete.\n");
  }
  // Child process
  else{
    // counter++;
    char *filename = micHandler_getRecordingPath();

    char duration_str[16];
    snprintf(duration_str, sizeof(duration_str), "%d", duration);

    char *args[] = {
      "arecord",
      "-D", MIC_DEVICE,
      "-f", MIC_FORMAT,
      "-r", MIC_RATE,
      "-c", "2",
      "--duration", duration_str,
      filename,
      NULL //needs to be null terminated
    };

    execvp("arecord", args);
    perror("Error when executing arecord");
    free(filename);
    exit(EXIT_FAILURE);
  }
}