#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_

#include <Track.h>

#define UI_TASK_NAME "UI_TASK_NA\0"
#define PROMPT_CHAR  2
#define UPDATE_SENSOR 3
#define UPDATE_SWITCH 4
#define UPDATE_TRAIN  5
#define UPDATE_TIME   6
#define UPDATE_IDLE   7
#define DEBUG_MSG     8

typedef struct UiMsg {
  char type;   //
  char data1;  // Row (2 digit allowed), or sensor box
  char data2;  // Col (2 digit allowed), or sensor val
  int data3;  // time
} UiMsg;

typedef struct TrainUiMsg {
  char type;       // always UPDATE_TRAIN  (used for consistency with UiMsg)
  char nth;

  char lastSensorUnexpected;
  char lastSensorBox;
  char lastSensorVal;
  int lastSensorActualTime;
  int lastSensorPredictedTime;

  char speed;      // 0 - 14
  char speedDir;
  int velocity;   // mm/s
  int distanceFromLastSensor;
  int distanceToNextSensor;

  char nextSensorBox;
  char nextSensorVal;
  int nextSensorPredictedTime;

  int lastSensorDistanceError;

  Position destination;
} TrainUiMsg;

int startUserInterfaceTask();

#define PrintDebug(tid, fmt, ...) \
{ \
  char DebugBuffer[128]; \
  DebugBuffer[0] = DEBUG_MSG; \
  int DebugLen = sprintff(DebugBuffer+1, fmt, ##__VA_ARGS__ ); \
  Send(tid, DebugBuffer, DebugLen + 1, (char*)1, 0);  \
}

#endif // USER_INTERFACE_H_
