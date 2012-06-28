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

#define REROUTE -1
// TODO, smaller Delay increments. and make better guess.

static int com1;
static int com2;

static int getStoppingDistance(Driver* me) {
  return me->d[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir][MAX_VAL];
}

// mm/s
static int getVelocity(Driver* me){
  return me->v[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir];
}

static void getRoute(Driver* me, DriverMsg* msg) {
  TrackMsg trackmsg;
  trackmsg.type = ROUTE_PLANNING;
  trackmsg.landmark1 = msg->landmark1;
  trackmsg.landmark2 = msg->landmark2;

  Send(me->trackManager, (char*)&trackmsg,
      sizeof(TrackMsg), (char*)&(me->route), sizeof(Route));
  me->routeRemaining = 0;

  PrintDebug(me->ui, "Distance %d \n", me->route.dist);
  PrintDebug(me->ui, "Num Node %d \n", me->route.length);

  for (int i = 0; i < me->route.length; i++) {
    RouteNode node = me->route.nodes[i];
    if (node.num == -1) {
      PrintDebug(me->ui, "reverse \n");
    } else {
      PrintDebug(me->ui, "Landmark %d %d %d %d", node.landmark.type, node.landmark.num1, node.landmark.num2, node.dist);
      if (node.landmark.type == LANDMARK_SWITCH && node.landmark.num1 == BR) {
        PrintDebug(me->ui, "Switch %d ", node.num);
      }
    }
  }
}

static void setRoute(Driver* me) {
  // Find the first reverse on the path, stop if possible.
  int reverseNode = me->route.length-1;
  PrintDebug(me->ui, "Hm %d\n", me->routeRemaining);
  PrintDebug(me->ui, "Len %d\n", me->route.length);
  for (int i = me->routeRemaining; i < me->route.length; i++) {
    if (me->route.nodes[i].num == REVERSE) {
      reverseNode = i;
      break;
    }
    else if (me->route.nodes[i].landmark.type == LANDMARK_SWITCH &&
        me->route.nodes[i].landmark.num1 == BR) {
      TrackMsg setSwitch;
      setSwitch.type = SET_SWITCH;
      setSwitch.data = me->route.nodes[i].num;
      Send(me->trackManager, (char*)&setSwitch, sizeof(TrackMsg), (char*)NULL, 0);
    }
  }

  int stop = getStoppingDistance(me);
  int delayTime = 0;
  // Find the stopping distance for the reverseNode.
  //for (int i = reverseNode; i >= 0; i--) {
  //  stop -= me->route.nodes[i].dist;
  //  if (stop < 0) {
  //    delayTime = -stop*100 / getVelocity(me);
  //    Send(me->navigateNagger, (char*)&delayTime, 4,(char*)1, 0);
  //  }
  //}

  //// Gonna go pass the place.
  //if (delayTime == 0) {
  //  delayTime = -500; // Reroute after 500ms
  //  Send(me->navigateNagger, (char*)&delayTime, 4,(char*)1, 0);
  //} else {
  //  me->routeRemaining = reverseNode;
  //}
}

static void dynamicCalibration(Driver* me) {
  if (me->uiMsg.lastSensorUnexpected) return;

  int dTime = me->uiMsg.lastSensorPredictedTime - me->uiMsg.lastSensorActualTime;
  if (dTime > 1000) return;
  int velocity = me->calibrationDistance / (me->uiMsg.lastSensorActualTime - me->calibrationStart);
  int originalVelocity = me->v[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir];
  me->v[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir]
      = 0.5 * originalVelocity + 0.5 * velocity;
}

static void trainSetSpeed(DriverMsg* origMsg, Driver* me) {
  const int trainNum = origMsg->trainNum;
  me->trainNum = trainNum;
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

      // Modify prediction
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
  }
}

static void trainUiNagger() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = UI_NAGGER;
  for (;;) {
    Delay(20, timeserver); // .2 seconds
    msg.timestamp = Time(timeserver);
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

static void trainNavigateNagger() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent;

  int delay;
  DriverMsg msg;
  msg.type = NAVIGATE_NAGGER;
  for (;;) {
    Receive(&parent, (char*)&delay, 4);

    msg.data2 = (delay < 0) ? REROUTE : delay; // Re-Navigate
    if (delay < 0) delay = -delay;
    Delay(delay/10, timeserver);
    msg.timestamp = Time(timeserver);
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

static void initDriver(Driver* me) {
  char uiName[] = UI_TASK_NAME;
  me->ui = WhoIs(uiName);
  char trackName[] = TRACK_NAME;
  me->trackManager = WhoIs(trackName);
  me->route.length = 0;

  me->uiMsg.speed = 0;
  me->uiMsg.speedDir = ACCELERATE;

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
    int dPosition = (time - me->reportTime) * getVelocity(me) /10000;
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
        msg.timestamp = 300; // Delay for 3s
        trainSetSpeed(&msg, &me);
        if (msg.data3 != DELAYER) {
          //printff(com2, "Replied to %d\n", replyTid);
          Reply(replyTid, (char*)1, 0);
          sendUiReport(&me, 0); // Don't update time
          break;
        }
      }
      case DELAYER: {
        // Worker reporting back.
        break;
      }
      case UI_NAGGER: {
        sendUiReport(&me, 0);
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
        me.uiMsg.lastSensorPredictedTime = me.uiMsg.nextSensorPredictedTime;
        dynamicCalibration(&me);

        TrackNextSensorMsg tMsg;
        TrackMsg qMsg;
        qMsg.type = QUERY_NEXT_SENSOR;
        qMsg.landmark1.type = LANDMARK_SENSOR;
        qMsg.landmark1.num1 = me.uiMsg.lastSensorBox;
        qMsg.landmark1.num2 = me.uiMsg.lastSensorVal;
        Send(me.trackManager, (char*)&qMsg, sizeof(TrackMsg),
              (char*)&tMsg, sizeof(TrackNextSensorMsg));
        me.calibrationStart = msg.timestamp + 40; // 40ms for sending data to train
        me.calibrationDistance = tMsg.dist;
        me.uiMsg.distanceFromLastSensor = 0;
        me.uiMsg.distanceToNextSensor = tMsg.dist;
        me.uiMsg.nextSensorBox = tMsg.sensor.num1;
        me.uiMsg.nextSensorVal = tMsg.sensor.num2;
        me.uiMsg.nextSensorPredictedTime =
          msg.timestamp + me.uiMsg.distanceToNextSensor*100000 / getVelocity(&me) - 50; // 50 ms delay for sensor query.

        sendUiReport(&me, msg.timestamp);
        break;
      }
      //case NAVIGATE_NAGGER: {
      //  if (msg.data2 == REROUTE) {
      //    setRoute(&me);
      //  } else {
      //    // Want to stop now.
      //    msg.data2 = 0;  // Set speed zero.
      //    msg.timestamp = 2*getStoppingDistance(&me) / getVelocity(&me); // Time to stop
      //    trainSetSpeed(&msg, &me);
      //  }
      //  break;
      //}
      case SET_ROUTE: {
        getRoute(&me, &msg);
        setRoute(&me);
        trainSetSpeed(&msg, &me);
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
