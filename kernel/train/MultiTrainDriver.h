#ifndef MULTITRAIN_H_
#define MULTITRAIN_H_

#include <Driver.h>
#include <DumbDriver.h>

#define NUM_PREVIOUS_SENSOR 2
#define MAX_TRAIN_IN_GROUP 2

typedef struct MultiTrainDriver {
  // the single train driver side of multi-train driver,
  // most parameters relates to the head train
  Driver driver;

  // The multi-train driver side
  int numTrainInGroup;
  int trainId[MAX_TRAIN_IN_GROUP];
  DumbDriverInfo info[MAX_TRAIN_IN_GROUP];

  TrackLandmark previousSensor[NUM_PREVIOUS_SENSOR];
  int previousSensorCount[NUM_PREVIOUS_SENSOR];
} MultiTrainDriver;

typedef struct MultiTrainDriverMsg {
  char trainNum;
  char type; // 80 and up only
  char replyTid;       // The user that first send the message.
} MultiTrainDriverMsg;

void dumb_driver();

#endif
