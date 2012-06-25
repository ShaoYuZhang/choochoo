#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_

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
  char lastSensor;
  char speed;      // -14 -> 14
  char velocity;   // mm/s
  char nextSensor;

  int lastSensorTime;
  int nextSensorTime;
  int lastPredictionTime;
  int distanceFromLastSensor;
} TrainUiMsg;



int startUserInterfaceTask();

#endif // USER_INTERFACE_H_
