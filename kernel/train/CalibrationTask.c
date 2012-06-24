#include "CalibrationTask.h"
#include "Train.h"
#include <IoServer.h>
#include <IoHelper.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <Sensor.h>
#include <kernel.h>

#define AVG_COUNT 1
static int com1;
static int com2;
static int trainController;
static int timeserver;
static int sensorServer;
static SensorMsg sensorMsg;
static TrainMsg setSpeed;


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
    Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char *)NULL, 0);
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
        Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char*)NULL, 0);
        Delay(550, timeserver);
      }
      setSpeed.data2 = speed;
      Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char*)NULL, 0);
      while (1) {
        Sensor sensor;
        int tid;
        Receive(&tid, (char*)&sensor, sizeof(Sensor));
        Reply(tid, (char *)1, 0);
        //printff(com2, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
        if (sensor.box == startBox && sensor.val == startVal) {
          setSpeed.data2 = 0;
          Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char*)NULL, 0);
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

void goA(char train) {}

void goB(char train) {

  Putstr(com2, "Calibrating Velocity\n", 21);
  // Change track for velocity calibration
  TrainMsg setSwitch;
  setSwitch.type = SET_SWITCH;
  setSwitch.data2 = SWITCH_STRAIGHT;
  setSwitch.data1 = 16; // Switch 16
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);
  setSwitch.data1 = 17; // Switch 17
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);
  setSwitch.data1 = 9;  // Switch 9
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);

  // Message init
  setSpeed.type = SET_SPEED;
  setSpeed.data1 = (char)train;
  sensorMsg.type = QUERY_RECENT;
  Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
      (char*)1, 0);

  calibrateVelocity(1, 4, 8, 2, 14, 7850000);
  calibrateVelocity(0, 4, 8, 2, 14, 7850000);

  calibrateStopping(1, 4, 8);
  calibrateStopping(0, 4, 8);

  setSwitch.data2 = SWITCH_CURVED;
  setSwitch.data1 = 16; // Switch 16
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);
  setSwitch.data1 = 13; // Switch 13
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);
  setSwitch.data1 = 14; // Switch 14
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);
  setSwitch.data1 = 153; // Switch 153, 0x99
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);
  setSwitch.data1 = 156; // Switch 156, 0x9c
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);

  calibrateVelocity(1, 1, 3, 4, 15, 9390000);
  calibrateVelocity(0, 1, 3, 4, 15, 9390000);

  calibrateStopping(1, 1, 3);
  calibrateStopping(0, 1, 3);

  // Stopping train.
  Putstr(com2, "Stopping train      \n", 21);
  setSpeed.data2 = 0;
  Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char *)NULL, 0);
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
  goB(43);
  return;

  char track = Getc(com2);

  Putstr(com2, "Which train?\n", 13);
  char train = 10*(Getc(com2) - '0') + (Getc(com2) - '0');

  if (track == 'a') {
    Putstr(com2, "Using track a\n", 14);
    goA(train);
  }
  else {
    Putstr(com2, "Using track b\n", 14);
    goB(train);
  }
}

int startCalibrationTask() {
  return Create(10, calibration);
}
