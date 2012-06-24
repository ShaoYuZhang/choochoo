#include "CalibrationTask.h"
#include "Driver.h"
#include <Track.h>
#include <IoServer.h>
#include <IoHelper.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <Sensor.h>
#include <kernel.h>

#define AVG_COUNT 3
static int com1;
static int com2;
static int trainController;
static int trackController;
static int timeserver;
static int sensorServer;
static SensorMsg sensorMsg;
static DriverMsg setSpeed;


void calibrateVelocity(int accending, char startBox, char startVal, char endBox, char endVal, int distance) {
  char speed = 0;
  if (accending) {
    speed = 5;
  } else {
    speed = 14;
  }
  while (1) {
    int startTime = -1;
    setSpeed.data2 = speed;
    setSpeed.data3 = -1;
    Putstr(com2, "New\n", 4);
    Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char *)NULL, 0);
    for (int avgCount = 0; avgCount < AVG_COUNT; avgCount++) {
      while (1) {
        Sensor sensor;
        int tid;
        Receive(&tid, (char *)&sensor, sizeof(Sensor));
        Reply(tid, (char *)1, 0);
        //printff(com2, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
        if (startTime != -1 && sensor.box == endBox && sensor.val == endVal) {
          printff(com2, "Exiting %d mm/s:%d\n", sensor.time, distance / (sensor.time - startTime) );
          break;
        } else if (sensor.box == startBox && sensor.val == startVal) {
          printff(com2, "Got time %d \n", sensor.time);
          startTime = sensor.time;
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
      if (speed == 4) {
        break;
      }
    }
  }
}

void calibrateStopping(int accending, char startBox, char startVal) {
  Putstr(com2, "STOP DISTANCE CALI  \n", 21);
  char speed = 0;
  if (accending) {
    speed = 5;
  } else {
    speed = 14;
  }

  while(1) {
    setSpeed.data3 = -1;
    Putstr(com2, "New\n", 4);
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
        //printff(com2, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
        if (sensor.box == startBox && sensor.val == startVal) {
          setSpeed.data2 = 0;
          Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char*)NULL, 0);
          Putstr(com2, "STOP\n", 5);
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
      if (speed == 4) {
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

  Putstr(com2, "Calibrating Velocity\n", 21);
  // Change track for velocity calibration
  TrackMsg setSwitch;
  setSwitch.type = SET_SWITCH;
  setSwitch.data = SWITCH_STRAIGHT;

  TrackLandmark sw;
  sw.type = LANDMARK_SWITCH;
  sw.num1 = 0;
  sw.num2 = 16; // Switch 16
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(DriverMsg), (char *)NULL, 0);
  sw.num2= 17; // Switch 17
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(DriverMsg), (char *)NULL, 0);
  sw.num2= 9;  // Switch 9
  setSwitch.landmark1 = sw;
  Send(trackController, (char*)&setSwitch, sizeof(DriverMsg), (char *)NULL, 0);

  // Message init
  setSpeed.type = SET_SPEED;
  setSpeed.data1 = (char)train;
  sensorMsg.type = QUERY_RECENT;
  Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
      (char*)1, 0);

#if 0
  calibrateVelocity(1, sStartBox, sStartVal, sEndBox, sEndVal, sDistance);
  calibrateVelocity(0, sStartBox, sStartVal, sEndBox, sEndVal, sDistance);

  calibrateStopping(1, sStartBox, sStartVal);
  calibrateStopping(0, sStartBox, sStartVal);
#endif

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

  // Stopping train.
  Putstr(com2, "Stopping train      \n", 21);
  setSpeed.data2 = 0;
  Send(trainController, (char*)&setSpeed, sizeof(DriverMsg), (char *)NULL, 0);
  Putstr(com2, "done                \n", 21);
  Delay(500, timeserver);
  kernel_quit();
}

void calibration() {
  char com2Name[] = IOSERVERCOM2_NAME;
  char com1Name[] = IOSERVERCOM1_NAME;
  char trainControllerName[] = TRAIN_NAME;
  char timename[] = TIMESERVER_NAME;
  char sensorName[] = SENSOR_NAME;
  sensorServer = WhoIs(sensorName);
  timeserver = WhoIs(timename);
  trainController = WhoIs(trainControllerName);
  com1 = WhoIs(com1Name);
  com2 = WhoIs(com2Name);
  Putstr(com2, "Assume all tracks are curved. Which track?\n", 42);
  go(37,
        // Straight stuff
       4,8, 2,14, 7850000,
        // Curve stuff
       1,3, 4,15, 8950000
      );
  return;

  char track = Getc(com2);

  Putstr(com2, "Which train?\n", 13);
  char train = 10*(Getc(com2) - '0') + (Getc(com2) - '0');

  if (track == 'a') {
    Putstr(com2, "Using track a\n", 14);
    go(train,
        // Straight stuff
       2,15, 4,11, 6880000,
        // Curve stuff
       1,3, 4,15, 8950000
      );
  }
  else {
    Putstr(com2, "Using track b\n", 14);
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
