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

typedef struct CaliMsg {
  char sensorBox;
  char sensorVal;
} CaliMsg;

static void sensorQuery() {
  int parent = MyParentsTid();
  char sensorName[] = SENSOR_NAME;
  int sensorServer = WhoIs(sensorName);
  printff(com2, "Starting Sensor Query\n");

  for (;;) {
    SensorMsg sensorMsg;
    sensorMsg.type = QUERY_RECENT;
    Sensor sensor;
    Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
        (char*)&sensor, sizeof(Sensor));
    Send(parent, (char*)&sensor, sizeof(Sensor), (char*)1, 0);
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

  Create(1, sensorQuery);

  // Setting train speed.
  TrainMsg setSpeed;
  setSpeed.type = SET_SPEED;
  setSpeed.data1 = (char)train;
  setSpeed.data2 = (char)10;
  setSpeed.data3 = -1;
  Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char *)NULL, 0);
  Delay(800, timeserver);

  int startTime = -1;
  // See sensor reports and estimate velocity.
  for (int speed = 15; speed > 10; speed--) {
    printff(com2, "Calibrating Velocity %d\n", speed);
    setSpeed.data2 = (char)speed;
    setSpeed.data3 = -1;
    Delay(300, timeserver);
    //Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char *)NULL, 0);
    //int tid = -1;
    //while (1) {
    //  Sensor sensor;
    //  Receive(&tid, &sensor, sizeof(Sensor));
    //  Reply(tid, (char*)1, 0);
    //  printff(com2, "S:%d %d %d\n", sensor.box, sensor.val, sensor.time);
    //  if (startTime != -1 && sensor.box == 4 && sensor.val == 8) {
    //    printff(com2, "exiting\n");
    //    break;
    //  } else if (sensor.box == 2 && sensor.val == 14) {
    //    printff(com2, "Got time\n");
    //    startTime = sensor.time;
    //  }
    //}
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
