#ifndef USER_INTERFACE_H_
#define USER_INTERFACE_H_

#define UI_TASK_NAME "UI_TASK_NA\0"
#define PROMPT_CHAR  2
#define UPDATE_SENSOR 3
#define UPDATE_SWITCH 4
#define UPDATE_TRAIN  5
#define UPDATE_TIME   6
#define UPDATE_IDLE   7

typedef struct UiMsg {
  char type;   //
  char data1;  // Row (2 digit allowed)
  char data2;  // Col (2 digit allowed)
  int data3;  // time
} UiMsg;

int startUserInterfaceTask();

#endif // USER_INTERFACE_H_
