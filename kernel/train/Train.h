#ifndef TRAIN_H_
#define TRAIN_H_

#define TRAIN_NAME "TRAIN\0"
#define NUM_SWITCHES 0xff
#define NUM_TRAINS 80

int startTrainController();

void trainClose();

void trainGetSwitch();

void trainSetSwitch();

void trainSetSpeed();

#endif // TRAIN_H_
