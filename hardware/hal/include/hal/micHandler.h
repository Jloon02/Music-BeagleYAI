// terminal_output.h
// outputs information about sounds every second

#ifndef _MIC_HANDLER_OUTPUT_H_
#define _MIC_HANDLER_OUTPUT_H_

void micHandler_startRecording(int duration);

// The user is in charge of Freeing the value returned
char* micHandler_getRecordingPath();

#endif