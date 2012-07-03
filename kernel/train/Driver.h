#ifndef TRAIN_H_
#define TRAIN_H_

#include <Track.h>
#include <UserInterface.h>
#include <Poly.h>

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
#define BROADCAST_UPDATE_PREDICATION 8

typedef struct DriverInitMsg {
  int nth;
  int trainNum;
} DriverInitMsg;

typedef struct DriverMsg {
  char trainNum; // DONT USE IN DRIVER.c use me->trainNum
  char type;           // As defined above
  signed char data2;   // Speed (-1 for reverse)
  unsigned char data3; // Delay num, or msg came from worker
  char replyTid;       // The user that first send the message.
  int timestamp;
  Position pos;
} DriverMsg;


typedef struct Driver {
  int trainNum;
  int delayer;
  int ui;        // Ui Tid
  int sensorWatcher;
  int trackManager;
  int navigateNagger;
  int timeserver;
  int reportTime;
  int calibrationStart;
  int calibrationDistance;
  int routeRemaining;
  int stopNode;
  int stopCommited;
  int stopSensorHit;
  int stopSensorBox;
  int stopSensorVal;
  int distancePassStopSensorToStop;
  int useLastSensorNow;

  TrainUiMsg uiMsg;
  Route route;

  // Actual Stuf
  char lastSensorUnexpected;
  char lastSensorBox;
  char lastSensorVal;
  int lastSensorActualTime;
  int lastSensorPredictedTime;

  int speed;      // 0 - 14
  int speedDir;
  int distanceFromLastSensor;
  int distanceToNextSensor;
  char justReversed;

  char nextSensorBox;
  char nextSensorVal;
  int nextSensorPredictedTime;

  int lastSensorDistanceError;

  // A/D stuff
  int isAding;
  int distanceFromSensorAtStartAD;
  int distanceToSensorAtStartAD;
  int lastReportDist;
  int adEndTime;
  Poly adPoly;

  int v[15][2];
  int d[15][2][2];
  int a[15]; // TODO should be 15 * 15 later
} Driver;

int startDriverControllerTask();

#endif // TRAIN_H_
