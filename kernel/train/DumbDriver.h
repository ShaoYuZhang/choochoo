#ifndef DUMB_DRIVER_H_
#define DUMB_DRIVER_H_

#include <Poly.h>
#include <DriverController.h>

typedef struct DumbDriver {
  int trainNum;

  // Task Id
  int multiTrainController;
  int timeserver;
  int delayer;
  int stopDelayer;
  int ui;
  int com1;
  int trackManager;
  int navigateNagger;
  int courier;

  int lastPosUpdateTime;
  int calibrationStart;
  int calibrationDistance;
  int speedAfterReverse;

  // Actual Stuf
  char lastSensorIsTerminal;
  char lastSensorUnexpected;
  char lastSensorBox;
  char lastSensorVal;
  int lastSensorActualTime;

  int speed;      // 0 - 14
  int speedDir;
  float distanceFromLastSensor;
  float distanceToNextSensor;
  int lastSensorDistanceError;
  int distanceToPreviousTrain;

  char nextSensorIsTerminal;
  char nextSensorBox;
  char nextSensorVal;
  int nextSensorPredictedTime;
  TrainUiMsg uiMsg;

  // prediction stuff
  TrackSensorPrediction predictions[20];
  int numPredictions;

  int reversed;
  int trainLen;
  int pickupOffset;

  // A/D stuff
  int isAding;
  float lastReportDist;
  int adEndTime;
  Poly adPoly;

  int v[15][2];
  int d[15][2][2];
  int a[15]; // TODO should be 15 * 15 later
} DumbDriver;

typedef struct DumbDriverInfo {
  int trainSpeed;
  int velocity;
  int maxStoppingDistance;
  int currentStoppingDistance;
  char lenFrontOfPickup;
  char lenBackOfPickup;
  Position pos;
} DumbDriverInfo;

int CreateDumbTrain(int nth, int trainNum);

void DumbTrainSetSpeed(int tid, int speed);

#endif // DUMB_DRIVER_H_
