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
#define MERGE_TAIL 103
#define SEPARATE_TAIL 104
#define QUERY_STOP_COUNT 105
#define MULTI_TRAIN_DRIVER_COURIER 106
#define GET_POSITION 107

typedef struct MultiTrainDriver {
  int trainNum;
  int speed;      // 0 - 14
  int speedDir;
  int timeserver;
  int trainController;
  int sensorWatcher;
  // the single train driver side of multi-train driver,
  // most parameters relates to the head train
  //Driver driver;
  int infoUpdater;
  int stopNode;
  int previousStopNode;
  int distanceFromLastSensorAtPreviousStopNode;
  int stopCommited;
  int stopSensorHit;
  int stopSensorBox;
  int stopSensorVal;
  int distancePassStopSensorToStop;
  int useLastSensorNow;
  int stopNow;
  int positionFinding;
  int rerouteCountdown;
  int nextSetSwitchNode;
  int setSwitchNaggerCount;
  int testMode;
  Route route;

  // The multi-train driver side
  int ui;
  int trackManager;
  int courier;
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

  int v[15][2];
  int d[15][2][2];
  int a[15]; // TODO should be 15 * 15 later
  int CC;
  int routeRemaining;
  char nextSensorIsTerminal;
  char nextSensorBox;
  TrackLandmark reserveFailedLandmark;
  char nextSensorVal;
  int nextSensorPredictedTime;



  DriverMsg routeMsg;
} MultiTrainDriver;

typedef struct MultiTrainDriverMsg {
  char trainNum;
  char type; // 80 and up only
  char replyTid;       // The user that first send the message.
  int data;
  int numSensors;
  TrackLandmark sensors[10];
} MultiTrainDriverMsg;

typedef struct MultiTrainDriverCourierMsg {
  int destTid;
  MultiTrainDriverMsg msg;
} MultiTrainDriverCourierMsg;

void dumb_driver();

int createMultitrainDriver(int nth, int trainNum);

void RegisterMulti(int trainNum);
int  WhoIsMulti(int trainNum);

#endif
