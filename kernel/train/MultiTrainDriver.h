#ifndef MULTITRAIN_H_
#define MULTITRAIN_H_

#include <Driver.h>
#include <DumbDriver.h>

#define MAX_PREVIOUS_SENSOR 5
#define MAX_TRAIN_IN_GROUP 5

//Multi train driver msg types, start from 100
#define INFO_UPDATE_NAGGER 100
#define UPDATE_PREDICTION 101
#define STOP_COMPLETED 102

typedef struct MultiTrainDriver {
  // the single train driver side of multi-train driver,
  // most parameters relates to the head train
  Driver driver;

  // The multi-train driver side
  int numTrainInGroup;
  int isReversing;
  int speedAfterReverse;
  int trainId[MAX_TRAIN_IN_GROUP];
  DumbDriverInfo info[MAX_TRAIN_IN_GROUP];
  int stoppedCount;

  // An array of sensors to reserve for each train in group
  TrackLandmark sensorToReserve[MAX_TRAIN_IN_GROUP][10];
  int numSensorToReserve[MAX_TRAIN_IN_GROUP];

  int tailMode;
  int headTid;
} MultiTrainDriver;

typedef struct MultiTrainDriverMsg {
  char trainNum;
  char type; // 80 and up only
  char replyTid;       // The user that first send the message.
  int data;
  int numSensors;
  TrackLandmark sensors[10];
} MultiTrainDriverMsg;

void dumb_driver();

int createMultitrainDriver(int nth, int trainNum);

void RegisterMulti(int trainNum);
int  WhoIsMulti(int trainNum);

#endif
