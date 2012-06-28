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
static int ui;

static int getStoppingDistance(Driver* me) {
  return me->d[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir][MAX_VAL];
}

// mm/s
static int getVelocity(Driver* me){
  return me->v[(int)me->uiMsg.speed][(int)me->uiMsg.speedDir];
}

static int getStoppingTime(Driver* me) {
  return 2* getStoppingDistance(me) * 100000 / getVelocity(me);
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

static void updateTimeToSendStop(Driver* me, int speed) {
  const int stoppingDistance =
      me->d[speed][(int)me->uiMsg.speedDir][MAX_VAL];
  int stop = stoppingDistance;
  int delayTime = 0;

  // Find the stopping distance for the stopNode.
  // S------L------L---|-----L---------R------F
  //                   |__stop_dist____|
  // |__travel_dist____|
  // |delay this much..|
  //PrintDebug(me->ui, "Stop at Node %d starts %d \n",
  //    me->stopNode, me->routeRemaining);
  for (int i = me->stopNode; i >= me->routeRemaining; i--) {
    // PrintDebug(me->ui, "Stop %d \n", stop);
    // TODO, incorporate calculating node distance with distance to next node.
    stop -= me->route.nodes[i].dist;
    if (stop < 0) {
      int travelDistance = -me->route.nodes[i].dist;
      for(int j = i-1; j >= 0; j--) {
        ASSERT(j >= 0, "Bad travel dstiance." );
        travelDistance += me->route.nodes[j].dist;
      }
      int velocity = me->v[speed][(int)me->uiMsg.speedDir];
      delayTime = travelDistance * 100000 / velocity;

      PrintDebug(me->ui, "Got %d %d %d \n", delayTime, travelDistance, velocity);
      me->predictedTimeToStartStopping= Time(me->timeserver)*10 + delayTime - 40;
      break;
    }
  }

  // Gonna go pass the place.
  if (delayTime == 0) {
    PrintDebug(me->ui, "Cannot stop in time. \n");
    delayTime = 500; // Reroute after 500ms
    me->routeRemaining = -1;
  }
}

static void updateStopNode(Driver* me) {
  // Find the first reverse on the path, stop if possible.
  me->stopNode = me->route.length-1;
  for (int i = me->routeRemaining; i < me->route.length; i++) {
    if (me->route.nodes[i].num == REVERSE) {
      me->stopNode = i;
      break;
    }
    else if (me->route.nodes[i].landmark.type == LANDMARK_SWITCH &&
        me->route.nodes[i].landmark.num1 == BR) {
      TrackMsg setSwitch;
      setSwitch.type = SET_SWITCH;
      setSwitch.data = me->route.nodes[i].num;
      PrintDebug(me->ui, "set switch\n");
      Send(me->trackManager, (char*)&setSwitch, sizeof(TrackMsg), (char*)NULL, 0);
    }
  }
}

static void reRoute(Driver* me, char box, char val) {
}

// Update route traveled as sensors are hit.
static void updateRoute(Driver* me, char box, char val) {
  if (me->routeRemaining == -1) return;

  // See if we triggered
  PrintDebug(me->ui, "Update round: %d to %d!! \n", me->routeRemaining, me->stopNode);
  for (int i = me->routeRemaining; i < me->stopNode; i++) {
    if (me->route.nodes[i].landmark.type == LANDMARK_SENSOR &&
        me->route.nodes[i].landmark.num1 == box &&
        me->route.nodes[i].landmark.num2 == val)
    {
      PrintDebug(me->ui, "Triggered expected sensor!! %d\n", i);
      me->routeRemaining = i;
      updateTimeToSendStop(me, me->uiMsg.speed);
      break;
    }
  }

  // TODO if stoppped, update next stopNode.
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

static void trainSetSpeed(const int speed, const int stopTime, const int delayer, Driver* me) {
  char msg[4];
  msg[1] = (char)me->trainNum;
  if (speed >= 0) {
    if (delayer) {
      PrintDebug(me->ui, "Reversing speed. cuz its worker %d\n", speed);
      msg[0] = 0xf;
      msg[1] = (char)me->trainNum;
      msg[2] = (char)speed;
      msg[3] = (char)me->trainNum;
      Putstr(com1, msg, 4);

      // Modify prediction
    } else {
      PrintDebug(me->ui, "Set speed. %d %d\n", speed, me->trainNum);
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
    PrintDebug(me->ui, "Reverse... %d \n", me->uiMsg.speed);
    DriverMsg delayMsg;
    delayMsg.type = SET_SPEED;
    delayMsg.timestamp = stopTime;
    delayMsg.data2 = (signed char)me->uiMsg.speed;
    PrintDebug(me->ui, "Using delayer: %d for %d \n", me->delayer, stopTime);

    Reply(me->delayer, (char*)&delayMsg, sizeof(DriverMsg));

    msg[0] = 0;
    msg[1] = (char)me->trainNum;

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
    int numTick = msg.timestamp / 10;
    PrintDebug(ui, "Delay %d ticks\n", numTick);
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
    Delay(10, timeserver); // .2 seconds
    msg.timestamp = Time(timeserver) * 10;
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

static void trainNavigateNagger() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = NAVIGATE_NAGGER;
  for (;;) {
    Delay(9, timeserver); // .15 seconds
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

static void initDriver(Driver* me) {
  char uiName[] = UI_TASK_NAME;
  me->ui = WhoIs(uiName);
  ui = me->ui;
  char trackName[] = TRACK_NAME;
  me->trackManager = WhoIs(trackName);
  me->route.length = 0;
  me->stopCommited = 0; // haven't enabled speed zero yet.

  char timename[] = TIMESERVER_NAME;
  me->timeserver = WhoIs(timename);

  DriverInitMsg init;
  int controller;
  Receive(&controller, (char*)&init, sizeof(DriverInitMsg));
  Reply(controller, (char*)1, 0);
  me->trainNum = init.trainNum;
  me->uiMsg.nth = init.nth;

  me->uiMsg.speed = 0;
  me->uiMsg.speedDir = ACCELERATE;
  me->uiMsg.distanceToNextSensor = 0;
  me->uiMsg.distanceFromLastSensor = 0;

  me->delayer = Create(1, trainDelayer);
  me->uiNagger = Create(3, trainUiNagger);
  me->sensorWatcher = Create(3, trainSensor);
  me->navigateNagger = Create(2, trainNavigateNagger);
  me->routeRemaining = -1;

  me->uiMsg.type = UPDATE_TRAIN;

  initStoppingDistance((int*)me->d);
  initVelocity((int*)me->v);
}

static void sendUiReport(Driver* me, int time) {
  me->uiMsg.velocity = getVelocity(me) / 100;
  if (time) {
    // In mm
    int dPosition = (time - me->reportTime) * getVelocity(me) / 100000;
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
        trainSetSpeed(msg.data2,
                      getStoppingTime(&me),
                      (msg.data3 == DELAYER),
                      &me);
        if (msg.data3 != DELAYER) {
          //printff(com2, "Replied to %d\n", replyTid);
          Reply(replyTid, (char*)1, 0);
          sendUiReport(&me, 0); // Don't update time
          break;
        } else {
          // Reverse command completed
          me.stopCommited = 0; // We're moving again.
          // We've completed everything until the stop node.
          me.routeRemaining = me.stopNode;
          // Calculate the next stop node.
          updateStopNode(&me);
          updateTimeToSendStop(&me, msg.data2); // speed:
        }
      }
      case DELAYER: {
        PrintDebug(me.ui, "delayer come back.");
        break;
      }
      case UI_NAGGER: {
        sendUiReport(&me, msg.timestamp);
        break;
      }
      case SENSOR_TRIGGER: {
        if (msg.data2 != me.uiMsg.nextSensorBox || msg.data3 != me.uiMsg.nextSensorVal) {
          me.uiMsg.lastSensorUnexpected = 1;
          //reRoute(&me, msg.data2, msg.data3); // TODO
        } else {
          me.uiMsg.lastSensorUnexpected = 0;
        }
        updateRoute(&me, msg.data2, msg.data3);
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
        Send(me.trackManager, (char*)&qMsg, sizeof(TrackMsg),
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
      case NAVIGATE_NAGGER: {
        if (me.routeRemaining == -1) break;

        if (!me.stopCommited) {
          //updateTimeToSendStop(&me, me.uiMsg.speed); // speed:
          int deltaToStop = me.predictedTimeToStartStopping - Time(me.timeserver)*10;
          PrintDebug(me.ui, "Navi Nagger. %d", deltaToStop);
          if (deltaToStop < 50) {
            PrintDebug(me.ui, "Navi Nagger stopping %d.",
                me.predictedTimeToStartStopping);
            const int speed = 0;  // Set speed zero.
            trainSetSpeed(speed, getStoppingTime(&me), 0, &me);
            me.stopCommited = 1;
          }
        }
        //} else {
        //  // Do nothing.
        //}
        break;
      }
      case SET_ROUTE: {
        PrintDebug(me.ui, "Set route.");
        getRoute(&me, &msg);
        updateStopNode(&me);
        updateTimeToSendStop(&me, msg.data2); // speed:
        trainSetSpeed(msg.data2, 0, 0, &me);
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
      DriverInitMsg init;
      init.nth = nth;
      init.trainNum = (int)msg.trainNum;
      // Create train task
      trainTid[(int)msg.trainNum] = Create(4, driver);
      Send(trainTid[(int)msg.trainNum],
          (char*)&init, sizeof(DriverInitMsg), (char*)1, 0);
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