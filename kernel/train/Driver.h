#ifndef TRAIN_H_
#define TRAIN_H_

#include <Poly.h>
#include <DriverController.h>
#if 0

typedef struct Driver {
  int trainNum;
  int CC;

  // Task Id
  int trainController;
  int delayer;
  int stopDelayer;
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
  int currentlyLost;
  int testMode;

  TrackLandmark reserveFailedLandmark;
  TrainUiMsg uiMsg;
  Route route;

  DriverMsg routeMsg;

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

  char nextSensorIsTerminal;
  char nextSensorBox;
  char nextSensorVal;
  int nextSensorPredictedTime;

  int lastSensorDistanceError;
  int distanceToLongestSecondary;

  // prediction stuff
  TrackSensorPrediction predictions[20];
  int numPredictions;

  // A/D stuff
  int isAding;
  float lastReportDist;
  int adEndTime;
  Poly adPoly;

  int v[15][2];
  int d[15][2][2];
  int a[15]; // TODO should be 15 * 15 later
} Driver;


#endif
#endif // TRAIN_H_
