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

  for (;;) {
    SensorMsg sensorMsg;
    sensorMsg.type = QUERY_RECENT;
    Sensor sensor;
    Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
        (char*)&sensor, sizeof(Sensor));

    CaliMsg m;
    m.sensorBox = sensor.box;
    m.sensorVal = sensor.val;
    Send(parent, (char*)&m, sizeof(CaliMsg), (char*)1, 0);
  }
}

void goA(char train) {

}

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
  setSwitch.data1 = 9; // Switch 9
  Send(trainController, (char*)&setSwitch, sizeof(TrainMsg), (char *)NULL, 0);


  // Setting train speed.
  TrainMsg setSpeed;
  setSpeed.type = SET_SPEED;
  setSpeed.data1 = train;
  setSpeed.data2 = 10;
  Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char *)NULL, 0);
  Delay(500, timeserver); // 4 seconds
  Putstr(com2, "Reversing.\n", 11);
  setSpeed.data2 = -1;
  Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char *)NULL, 0);

  // See sensor reports and estimate velocity.
  Putstr(com2, "Calibrating Velocity\n", 21);
  Delay(1000, timeserver); // 4 seconds

  // Stopping train.
  Delay(500, timeserver); // 4 seconds
  Putstr(com2, "Stopping train      \n", 21);
  setSpeed.data2 = 0;
  Send(trainController, (char*)&setSpeed, sizeof(TrainMsg), (char *)NULL, 0);
  Putstr(com2, "done                \n", 21);
  kernel_quit();
}

void calibration() {
  char com2Name[] = IOSERVERCOM2_NAME;
  char com1Name[] = IOSERVERCOM1_NAME;
  char trainControllerName[] = TRAIN_NAME;
  char timename[] = TIMESERVER_NAME;
  timeserver = WhoIs(timename);
  Delay(700, timeserver); // 4 seconds
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
