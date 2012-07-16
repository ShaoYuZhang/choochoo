#ifndef DRIVER_CONTROLLER_
#define DRIVER_CONTROLLER_

#define TRAIN_CONTROLLER_NAME "TRAIN_CTRL\0"
#define NUM_TRAINS 80
#define SET_SPEED  0
#define GET_SPEED  1
#define GOTO_DEST  2
#define UI_NAGGER  3
#define DELAYER    4
#define SENSOR_TRIGGER 5
#define SET_ROUTE  6
#define NAVIGATE_NAGGER 7
#define BROADCAST_UPDATE_PREDICTION 8
#define FIND_POSITION  9
#define KNOW_POSITION  10
#define LOST_TRAIN     11
#define BROADCAST_LOST 12
#define STOP_DELAYER 13
#define BROADCAST_TEST_MODE 14

#include <UserInterface.h>
#include <Track.h>

typedef struct DriverMsg {
  char trainNum; // DONT USE IN DRIVER.c use me->trainNum
  char type;           // As defined above
  signed char data2;   // Speed (-1 for reverse)
  unsigned char data3; // Delay num, or msg came from worker
  char replyTid;       // The user that first send the message.
  int timestamp;
  Position pos;
} DriverMsg;

typedef struct DriverInitMsg {
  int nth;
  int trainNum;
  int com1;
} DriverInitMsg;

int startDriverControllerTask();

void BroadcastLost(int controllerTid);
void FinishPositionFinding(int trainNum, int controllerTid);

void driver();

#endif // DRIVER_CONTROLLER_
