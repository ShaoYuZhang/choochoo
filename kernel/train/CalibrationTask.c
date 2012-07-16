#include "CalibrationTask.h"
#include "Driver.h"
#include <Track.h>
#include <IoServer.h>
#include <IoHelper.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <Sensor.h>
#include <kernel.h>
#include <CalibrationData.h>

static int v[15][2];
static int d[15][2][2];

#define AVG_COUNT 2
static int trainController;
static int trackController;
static int timeserver;
static int sensorServer;
static SensorMsg sensorMsg;
static DriverMsg setSpeed;
static int ui;

void calibrateAccel(int accending, char startBox, char startVal, char endBox, char endVal, int distance) {
  char speed = 8;
  while (1) {
    setSpeed.data2 = 10;
    setSpeed.data3 = -1;
    PrintDebug(ui, "New Speed: %d\n", (int)speed);
    Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char *)NULL, 0);
    float timeTripSensor1 = -1;
    float timeSendCommand = -1;
    float timeTripSensor2 = -1;

    for (int avgCount = 0; avgCount < AVG_COUNT; avgCount++) {
      while (1) {
        Sensor sensor;
        int tid;
        Receive(&tid, (char *)&sensor, sizeof(Sensor));
        //printff(ui, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
        if (timeTripSensor1 != -1 && sensor.box == endBox && sensor.val == endVal) {
          timeTripSensor2 = sensor.time *10 - 50;

          float v1 =
         (float)v[(int)speed][ACCELERATE] / 100000.0;
          float v0 = (float)v[0][ACCELERATE] / 100000.0;
          float t1 = (-timeSendCommand +
            2.0*(v1*timeTripSensor2 - timeTripSensor1*v0 - (float)distance/10000)
            / (v1 - v0));

          setSpeed.data2 = 10;
          setSpeed.data3 = -1;
          Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char *)NULL, 0);

          PrintDebug(ui, "Using %d %d %d %d %d", (int)timeTripSensor1, (int)timeSendCommand, (int)timeTripSensor2, (int)(v0*1000), (int)(v1*1000))

          PrintDebug(ui, "Exiting %d T1:%d: Took:%d", sensor.time*10, (int)t1, (int)(t1-timeSendCommand));
          break;
        } else if (sensor.box == startBox && sensor.val == startVal) {
          timeTripSensor1 = sensor.time*10 - 50;

          setSpeed.data2 = speed;
          setSpeed.data3 = -1;
          PrintDebug(ui, "Got time %d \n", sensor.time);
          Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char *)NULL, 0);
          timeSendCommand = Time(timeserver)*10 + 50;
        } else if (sensor.box == 4 && sensor.val == 9) { // D10
          Delay(78, timeserver);
          setSpeed.data2 = 1;
          setSpeed.data3 = -1;
          PrintDebug(ui, "Stopping.%d \n", sensor.time*10);
          Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char *)NULL, 0);
        }
        Reply(tid, (char *)1, 0);
      }
    }

    speed += 1;
    if (speed == 10) {
      break;
    }
  }
}

void calibrateVelocity(int accending, char startBox, char startVal, char endBox, char endVal, int distance) {
  char speed = 0;
  if (accending) {
    speed = 5;
  } else {
    speed = 10;
  }
  while (1) {
    int startTime = -1;
    setSpeed.data2 = speed;
    setSpeed.data3 = -1;
    PrintDebug(ui, "New\n");
    Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char *)NULL, 0);
    for (int avgCount = 0; avgCount < AVG_COUNT; avgCount++) {
      while (1) {
        Sensor sensor;
        int tid;
        Receive(&tid, (char *)&sensor, sizeof(Sensor));
        Reply(tid, (char *)1, 0);
        //printff(ui, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
        if (startTime != -1 && sensor.box == endBox && sensor.val == endVal) {
          PrintDebug(ui, "Exiting %d mm/s:%d\n", sensor.time, distance / (sensor.time - startTime) );
          break;
        } else if (sensor.box == startBox && sensor.val == startVal) {
          PrintDebug(ui, "Got time %d \n", sensor.time);
          startTime = sensor.time;
        }
      }
    }
    if (accending) {
      speed += 1;
      if (speed == 11) {
        break;
      }
    } else {
      speed -= 1;
      if (speed == 4) {
        break;
      }
    }
  }
}

void calibrateStopping(int accending, char startBox, char startVal) {
  PrintDebug(ui, "STOP DISTANCE CALI  \n");
  char speed = 0;
  if (accending) {
    speed = 3;
  } else {
    speed = 10;
  }

  while (1) {
    setSpeed.data3 = -1;
    PrintDebug(ui, "New\n");
    for (int avgCount = 0; avgCount < AVG_COUNT; avgCount++) {
      if (!accending) {
        setSpeed.data2 = 14;
        Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char*)NULL, 0);
        Delay(550, timeserver);
      }
      setSpeed.data2 = speed;
      Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char*)NULL, 0);
      while (1) {
        Sensor sensor;
        int tid;
        Receive(&tid, (char*)&sensor, sizeof(Sensor));
        Reply(tid, (char *)1, 0);
        //printff(ui, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
        if (sensor.box == startBox && sensor.val == startVal) {
          setSpeed.data2 = 0;
          Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char*)NULL, 0);
          PrintDebug(ui, "STOP\n");
          if (speed < 12) {
            Delay(1000, timeserver);
          } else {
            Delay(2000, timeserver);
          }
          break;
        }
      }
    }
    if (accending) {
      speed += 1;
      if (speed == 15) {
        break;
      }
    } else {
      speed -= 1;
      if (speed == 2) {
        break;
      }
    }
  }
}

void go(char train,
        char sStartBox, char sStartVal,
        char sEndBox, char sEndVal, int sDistance,
        char cStartBox, char cStartVal,
        char cEndBox, char cEndVal, int cDistance) {

  PrintDebug(ui, "Calibrating start \n");
  // Change track for velocity calibration
  TrackMsg setSwitch;
  setSwitch.type = SUDO_SET_SWITCH;
  setSwitch.data = SWITCH_STRAIGHT;

  TrackLandmark sw;
  sw.type = LANDMARK_SWITCH;
  sw.num1 = 0;
  sw.num2 = 16; // Switch 16
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(TrackMsg), (char *)NULL, 0);
  sw.num2 = 17; // Switch 17
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(TrackMsg), (char *)NULL, 0);
  sw.num2 = 9;  // Switch 9
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(TrackMsg), (char *)NULL, 0);

  // Message init
  setSpeed.type = SET_SPEED;
  setSpeed.trainNum = (char)train;
  sensorMsg.type = QUERY_RECENT;
  Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
      (char*)1, 0);

  calibrateAccel(1, sStartBox, sStartVal, sEndBox, sEndVal, sDistance);
#if 0
  //calibrateVelocity(1, sStartBox, sStartVal, sEndBox, sEndVal, sDistance);
  //calibrateVelocity(0, sStartBox, sStartVal, sEndBox, sEndVal, sDistance);

  //calibrateStopping(1, sStartBox, sStartVal);
  calibrateStopping(0, sStartBox, sStartVal);
#endif

#if 0
  setSwitch.data = SWITCH_CURVED;
  sw.num2 = 16; // Switch 16
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(DriverMsg), (char *)NULL, 0);
  sw.num2 = 13; // Switch 13
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(DriverMsg), (char *)NULL, 0);
  sw.num2 = 14; // Switch 14
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(DriverMsg), (char *)NULL, 0);
  sw.num2= 153; // Switch 153, 0x99
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(DriverMsg), (char *)NULL, 0);
  sw.num2 = 156; // Switch 156, 0x9c
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(DriverMsg), (char *)NULL, 0);

  calibrateVelocity(1, cStartBox, cStartVal, cEndBox, cEndVal, cDistance);
  calibrateVelocity(0, cStartBox, cStartVal, cEndBox, cEndVal, cDistance);
  calibrateStopping(1, cStartBox, cStartVal);
  calibrateStopping(0, cStartBox, cStartVal);
#endif

  // Stopping train.
  PrintDebug(ui, "Stopping train      \n");
  setSpeed.data2 = 0;
  Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char *)NULL, 0);
  PrintDebug(ui, "done                \n");
  Delay(500, timeserver);
  kernel_quit();
}

void calibration() {
  char uiName[] = UI_TASK_NAME;
  char trainControllerName[] = TRAIN_CONTROLLER_NAME;
  char timename[] = TIMESERVER_NAME;
  char sensorName[] = SENSOR_NAME;
  char trackName[] = TRACK_NAME;
  sensorServer = WhoIs(sensorName);
  timeserver = WhoIs(timename);
  trainController = WhoIs(trainControllerName);
  trackController = WhoIs(trackName);
  ui = WhoIs(uiName);

  initStoppingDistance((int*)d);
  initVelocity((int*)v);

  PrintDebug(ui, "Assume all tracks are curved. Which track?\n");
  go(44,
        // Straight stuff
       4,8, 2,14, 7850000,
        // Curve stuff
       1,3, 4,15, 8950000
      );
  return;

  char track = Getc(ui);

  PrintDebug(ui, "Which train?\n");
  char train = 10*(Getc(ui) - '0') + (Getc(ui) - '0');

  if (track == 'a') {
    PrintDebug(ui, "Using track a\n");
    go(train,
        // Straight stuff
       2,15, 4,11, 6880000,
        // Curve stuff
       1,3, 4,15, 8950000
      );
  }
  else {
    PrintDebug(ui, "Using track b\n");
    go(train,
        // Straight stuff
       4,8, 2,14, 7850000,
        // Curve stuff
       1,3, 4,15, 8950000
      );
  }
}

int startCalibrationTask() {
  return Create(10, calibration);
}
