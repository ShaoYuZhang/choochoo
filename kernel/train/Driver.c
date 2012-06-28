#include "Driver.h"
#include <IoServer.h>
#include <ts7200.h>
#include <util.h>
#include <TimeServer.h>
#include <NameServer.h>
#include <IoHelper.h>
#include <syscall.h>
#include <CalibrationData.h>
#include <Sensor.h>
#include <Track.h>

static int com1;
static int com2;

static int getStoppingDistance(Driver* me) {
  return me->d[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir][MAX_VAL];
}

// mm/s
static int getVelocity(Driver* me){
  return me->v[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir];
}

static void dynamicCalibration(Driver* me) {
  if (me->uiMsg.lastSensorUnexpected) return;
  if (me->uiMsg.speed == 0) return; // Cannot calibrate speed zero

  int dTime = me->uiMsg.lastSensorPredictedTime - me->uiMsg.lastSensorActualTime;
  if (dTime > 1000) return;
  int velocity = me->calibrationDistance * 100 * 1000 / (me->uiMsg.lastSensorActualTime - me->calibrationStart);
  int originalVelocity = me->v[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir];
  me->v[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir]
      = (originalVelocity * 8 + velocity * 2) / 10;
}

static void trainSetSpeed(DriverMsg* origMsg, Driver* me) {
  const int trainNum = origMsg->trainNum;
  const int speed = origMsg->data2;

  char msg[4];
  msg[1] = (char)trainNum;
  if (speed >= 0) {
    if (origMsg->data3 == DELAYER) {
      printff(com2, "Reversing speed. cuz its worker %d\n", speed);
      msg[0] = 0xf;
      msg[1] = (char)trainNum;
      msg[2] = (char)speed;
      msg[3] = (char)trainNum;
      Putstr(com1, msg, 4);
    } else {
      printff(com2, "Set speed. %d %d\n", speed, trainNum);
      msg[0] = (char)speed;
      Putstr(com1, msg, 2);
    }
    if (speed > me->uiMsg.speed) {
      me->uiMsg.speedDir = ACCELERATE;
    } else if (speed < me->uiMsg.speed) {
      me->uiMsg.speedDir = DECELERATE;
    }
    me->uiMsg.speed = speed;
  } else {
    printff(com2, "Reverse... %d \n", me->uiMsg.speed);
    origMsg->data2 = (signed char)me->uiMsg.speed;
    // TODO, calculate from train speed.
    origMsg->data3 = 150; // YES IT IS 3s
    printff(com2, "Using delayer: %d \n", me->delayer);

    Reply(me->delayer, (char*)origMsg, sizeof(DriverMsg));

    msg[0] = 0;
    msg[1] = (char)trainNum;

    Putstr(com1, msg, 2);
    me->uiMsg.speed = 0;
    me->uiMsg.speedDir = DECELERATE;
  }
}

// It is SensorQuery's responsibility to return whether SensorServer's
// response is relevant to this Train.
//
// If the message from server is not for this train. It is dropped.
static void trainSensor() {
  int parent = MyParentsTid();
  char sensorName[] = SENSOR_NAME;
  int sensorServer = WhoIs(sensorName);

  DriverMsg msg;
  msg.type = SENSOR_TRIGGER;

  SensorMsg sensorMsg;
  sensorMsg.type = QUERY_RECENT;
  Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
      (char*)1, 0);
  for (;;) {
    Sensor sensor;
    int tid;
    Receive(&tid, (char *)&sensor, sizeof(Sensor));
    Reply(tid, (char *)1, 0);

    msg.data2 = sensor.box;
    msg.data3 = sensor.val;
    msg.timestamp = sensor.time * 10; // MS per tick

    // TODO:zhang confirm with prediction for multi-train setup.
    //  so that irrelevant triggers can be dropped.
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

// Responsider for delag msg server about positive train speed
static void trainDelayer() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = DELAYER;
  for (;;) {
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)&msg, sizeof(DriverMsg));
    int numTick = msg.data3; // num of 10ms
    numTick *= 2;
    Delay(numTick, timeserver);
    msg.data3 = DELAYER;
    //printff(com2, "Delayer Done. %d %d\n", numTick, parent);
  }
}

static void trainUiNagger() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = UI_NAGGER;
  for (;;) {
    Delay(10, timeserver); // .2 seconds
    msg.timestamp = Time(timeserver) * 10;
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

static void initDriver(Driver* me) {
  char uiName[] = UI_TASK_NAME;
  me->ui = WhoIs(uiName);

  char trackName[] = TRACK_NAME;
  me->track = WhoIs(trackName);

  me->uiMsg.speed = 0;
  me->uiMsg.speedDir = ACCELERATE;
  me->uiMsg.distanceToNextSensor = 0;
  me->uiMsg.distanceFromLastSensor = 0;

  me->delayer = Create(1, trainDelayer);
  me->uiNagger = Create(3, trainUiNagger);
  me->sensorWatcher = Create(3, trainSensor);
  int controller;
  Receive(&controller, (char*)&(me->uiMsg.nth), 4);

  me->uiMsg.type = UPDATE_TRAIN;

  initStoppingDistance((int*)me->d);
  initVelocity((int*)me->v);
}

static void sendUiReport(Driver* me, int time) {
  me->uiMsg.velocity = getVelocity(me) / 100;
  if (time) {
    // In mm
    int dPosition = (time - me->reportTime) * getVelocity(me) /100000;
    me->reportTime = time;
    me->uiMsg.distanceFromLastSensor += dPosition;
    me->uiMsg.distanceToNextSensor -= dPosition;
  }

  Send(me->ui, (char*)&(me->uiMsg), sizeof(TrainUiMsg), (char*)1, 0);
}

static void driver() {
  Driver me;
  initDriver(&me);

  for (;;) {
    int tid = -1;
    DriverMsg msg;
    msg.trainNum = -1;
    msg.data2 = -1;
    msg.data3 = -1;
    msg.replyTid = -1;
    Receive(&tid, (char*)&msg, sizeof(DriverMsg));
    if (tid != me.delayer) {
      Reply(tid, (char*)1, 0);
    }
    const int replyTid = msg.replyTid;

    switch (msg.type) {
      case GET_SPEED: {
        Reply(replyTid, (char*)&me.uiMsg.speed, 4);
        break;
      }
      case SET_SPEED: {
        trainSetSpeed(&msg, &me);
        if (msg.data3 != DELAYER) {
          //printff(com2, "Replied to %d\n", replyTid);
          Reply(replyTid, (char*)1, 0);
          sendUiReport(&me, 0); // Don't update time
          break;
        }
        // else fall through
        //printff(com2, "Worker setted speed%d\n", me.speed);
      }
      case DELAYER: {
        // Worker reporting back.
        break;
      }
      case UI_NAGGER: {
        sendUiReport(&me, msg.timestamp);
        break;
      }
      case SENSOR_TRIGGER: {
        if (msg.data2 != me.uiMsg.nextSensorBox || msg.data3 != me.uiMsg.nextSensorVal){
          me.uiMsg.lastSensorUnexpected = 1;
        } else {
          me.uiMsg.lastSensorUnexpected = 0;
        }
        me.uiMsg.lastSensorBox = msg.data2; // Box
        me.uiMsg.lastSensorVal = msg.data3; // Val
        me.uiMsg.lastSensorActualTime = msg.timestamp;
        dynamicCalibration(&me);
        me.uiMsg.lastSensorPredictedTime = me.uiMsg.nextSensorPredictedTime;

        TrackNextSensorMsg tMsg;
        TrackMsg qMsg;
        qMsg.type = QUERY_NEXT_SENSOR;
        qMsg.landmark1.type = LANDMARK_SENSOR;
        qMsg.landmark1.num1 = me.uiMsg.lastSensorBox;
        qMsg.landmark1.num2 = me.uiMsg.lastSensorVal;
        Send(me.track, (char*)&qMsg, sizeof(TrackMsg),
              (char*)&tMsg, sizeof(TrackNextSensorMsg));
        me.calibrationStart = msg.timestamp;
        me.calibrationDistance = tMsg.dist;
        me.uiMsg.distanceFromLastSensor = 0;
        me.uiMsg.distanceToNextSensor = tMsg.dist;
        me.reportTime = msg.timestamp;
        me.uiMsg.nextSensorBox = tMsg.sensor.num1;
        me.uiMsg.nextSensorVal = tMsg.sensor.num2;
        me.uiMsg.nextSensorPredictedTime =
          msg.timestamp + me.uiMsg.distanceToNextSensor*100000 / getVelocity(&me) - 50; // 50 ms delay for sensor query.

        sendUiReport(&me, msg.timestamp);
        break;
      }
      default: {
        ASSERT(FALSE, "Not suppported train message type.");
      }
    }
  }
}

// An admin that pass message to train.
// Additionally create train if it does not exist.
static void trainController() {
  char trainName[] = TRAIN_CONTROLLER_NAME;
  RegisterAs(trainName);

  char com1Name[] = IOSERVERCOM1_NAME;
  char com2Name[] = IOSERVERCOM2_NAME;
  com1 = WhoIs(com1Name);
  com2 = WhoIs(com2Name);

  int nth = 0;
  int trainTid[80]; // Train num -> train tid
  for (int i = 0; i < 80; i++) {
    trainTid[i] = -1;
  }

  for (;;) {
    int tid = -1;
    DriverMsg msg;
    msg.trainNum = -1;
    msg.data2 = -1;
    msg.data3 = -1;
    Receive(&tid, (char*)&msg, sizeof(DriverMsg));

    if (trainTid[(int)msg.trainNum] == -1) {
      // Create train task
      trainTid[(int)msg.trainNum] = Create(4, driver);
      Send(trainTid[(int)msg.trainNum], (char*)&nth, 4, (char*)1, 0);
      nth++;
    }

    msg.replyTid = (char)tid;
    // Pass the message on.
    Send(trainTid[(int)msg.trainNum], (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

int startDriverControllerTask() {
  return Create(5, trainController);
}
