#ifndef TRAIN_H_
#define TRAIN_H_

#define TRAIN_NAME "TRAINN\0"
#define NUM_SWITCHES 0xff
#define NUM_TRAINS 80
#define SWITCH_STRAIGHT 35
#define SWITCH_CURVED   34

#define GET_SWITCH 0
#define SET_SWITCH 1
#define SET_SPEED  2
#define GET_SPEED  3
#define WORKER     4

typedef struct TrainMsg {
  char type;  // Defined above
  char data1; // Train or Switch number
  signed char data2; // Speed or switch state (-1 for reverse)
  unsigned char data3; // Delay num, or msg came from worker
} TrainMsg;

int startTrainControllerTask();

void trainGetSwitch();

void trainSetSwitch(int switchNum, int state);

void trainSetSpeed(TrainMsg* origMsg, int* numWorkerLeft);

#endif // TRAIN_H_
