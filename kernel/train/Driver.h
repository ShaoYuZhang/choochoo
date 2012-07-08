#ifndef TRAIN_H_
#define TRAIN_H_

#include <Poly.h>
#include <DriverController.h>

typedef struct Driver {
  int trainNum;
  int CC;

  // Task Id
  int delayer;
  int ui;
  int com1;
  int sensorWatcher;
  int trackManager;
  int navigateNagger;
  int timeserver;

  int lastPosUpdateTime;
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

  int speedAfterReverse;

  // Actual Stuf
  char lastSensorIsTerminal;
  char lastSensorUnexpected;
  char lastSensorBox;
  char lastSensorVal;
  int lastSensorActualTime;
  int lastSensorPredictedTime;

  int speed;      // 0 - 14
  int speedDir;
  float distanceFromLastSensor;
  float distanceToNextSensor;
  char invalidLastSensor;

  char nextSensorIsTerminal;
  char nextSensorBox;
  char nextSensorVal;
  int nextSensorPredictedTime;

  int lastSensorDistanceError;

  // A/D stuff
  int isAding;
  float lastReportDist;
  int adEndTime;
  Poly adPoly;

  int v[15][2];
  int d[15][2][2];
  int a[15]; // TODO should be 15 * 15 later
} Driver;


#endif // TRAIN_H_
