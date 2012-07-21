#ifndef MULTITRAIN_H_
#define MULTITRAIN_H_

#include <Driver.h>
#include <DumbDriver.h>

#define NUM_PREVIOUS_SENSOR 5
#define MAX_TRAIN_IN_GROUP 5

//Multi train driver msg types, start from 100
#define INFO_UPDATE_NAGGER 100

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
