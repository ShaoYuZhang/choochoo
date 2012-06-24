#ifndef TRAIN_H_
#define TRAIN_H_

#define TRAIN_NAME "TRAINN\0"
#define NUM_TRAINS 80

#define SET_SPEED  0
#define GET_SPEED  1
#define WORKER     2

typedef struct TrainMsg {
  char type;  // Defined above
  char data1; // Train  number
  signed char data2; // Speed (-1 for reverse)
  unsigned char data3; // Delay num, or msg came from worker
} TrainMsg;

int startTrainControllerTask();

#endif // TRAIN_H_
