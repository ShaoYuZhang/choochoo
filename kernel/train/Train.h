#ifndef TRAIN_H_
#define TRAIN_H_

#define TRAIN_NAME "TRAINN\0"
#define NUM_SWITCHES 0xff
#define NUM_TRAINS 80
#define SWITCH_STRAIGHT 34
#define SWITCH_CURVED   35

#define GET_SWITCH 0
#define SET_SWITCH 1
#define SET_SPEED  2
#define GET_SPEED  3

typedef struct TrainMsg {
  char type;
  char data1; // Train or Switch number
  char data2; // Speed or switch state
} TrainMsg;

int startTrainControllerTask();

void trainClose();

void trainGetSwitch();

void trainSetSwitch(int switchNum, int state);

void trainSetSpeed(int trainNum, int spd);

#endif // TRAIN_H_
