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
  int ui;
  int com1;
  int navigateNagger;

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
  int lastSensorPredictedTime;

  int speed;      // 0 - 14
  int speedDir;
  float distanceFromLastSensor;
  float distanceToNextSensor;

  char nextSensorIsTerminal;
  char nextSensorBox;
  char nextSensorVal;
  int nextSensorPredictedTime;
  TrainUiMsg uiMsg;

  // A/D stuff
  int isAding;
  float lastReportDist;
  int adEndTime;
  Poly adPoly;

  int v[15][2];
  int d[15][2][2];
  int a[15]; // TODO should be 15 * 15 later
} DumbDriver;


#endif // DUMB_DRIVER_H_
