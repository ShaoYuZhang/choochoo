#include "CalibrationTask.h"
#include "Train.h"
#include <IoServer.h>
#include <IoHelper.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <Sensor.h>
#include <kernel.h>

static int com1;
static int com2;
static int trainController;
static int timeserver;
static int sensorServer;

typedef struct CaliMsg {
  char sensorBox;
  char sensorVal;
} CaliMsg;

static inline void getSensor(Sensor* s) {
  SensorMsg sensorMsg;
  sensorMsg.type = QUERY_RECENT;
  Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
      (char*)s, sizeof(Sensor));
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

  // Setting train speed.
  TrainMsg setSpeed;
  setSpeed.type = SET_SPEED;
  setSpeed.data1 = (char)train;

  int startTime = -1;
  // See sensor reports and estimate velocity.

  SensorMsg sensorMsg;
  sensorMsg.type = QUERY_RECENT;
  Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
      (char*)1, 0);

  for (char speed = 14; speed > 8; speed--) {
    setSpeed.data2 = speed;
    setSpeed.data3 = -1;
    Putstr(com2, "New\n", 4);
    Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char *)NULL, 0);
    for (int avgCount = 0; avgCount < 5; avgCount++) {
      while (1) {
        Sensor sensor;
        int tid;
        Receive(&tid, (char *)&sensor, sizeof(Sensor));
        Reply(tid, (char *)1, 0);
        //printff(com2, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
        if (startTime != -1 && sensor.box == 2 && sensor.val == 14) {
          printff(com2, "exiting %d\n", sensor.time);
          break;
        } else if (sensor.box == 4 && sensor.val == 8) {
          printff(com2, "Got time %d\n", sensor.time);
          startTime = sensor.time;
        }
      }
    }
  }

  Putstr(com2, "STOP DISTANCE CALI  \n", 21);
  for (char speed = 14; speed > 8; speed--) {
    setSpeed.data3 = -1;
    Putstr(com2, "New\n", 4);
    for (int avgCount = 0; avgCount < 3; avgCount++) {
      setSpeed.data2 = speed;
      Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char*)NULL, 0);
      while (1) {
        Sensor sensor;
        getSensor(&sensor);
        //printff(com2, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
        if (sensor.box == 4 && sensor.val == 8) {
          setSpeed.data2 = 0;
          Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char*)NULL, 0);
          Putstr(com2, "STOP\n", 5);
          Delay(1000, timeserver);
          break;
        }
      }
    }
  }


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
  goB(37);
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
