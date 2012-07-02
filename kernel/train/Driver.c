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
static int ui;
static int CC = 0;

static int getStoppingDistance(Driver* me) {
  return me->d[(int)me->speed][(int)me->speedDir][MAX_VAL];
}

// mm/s
static int getVelocity(Driver* me){
  return me->v[(int)me->speed][(int)me->speedDir];
}

static int getStoppingTime(Driver* me) {
  return 2* getStoppingDistance(me) * 100000 / getVelocity(me);
}

static void toPosition(Driver* me, Position* pos) {
  pos->landmark1.type = LANDMARK_SENSOR;
  pos->landmark1.num1 = me->lastSensorBox;
  pos->landmark1.num2 = me->lastSensorVal;
  pos->landmark2.type = LANDMARK_SENSOR;
  pos->landmark2.num1 = me->nextSensorBox;
  pos->landmark2.num2 = me->nextSensorVal;
  pos->offset = me->distanceFromLastSensor;
}

static void getRoute(Driver* me, DriverMsg* msg) {
  TrackMsg trackmsg;
  trackmsg.type = ROUTE_PLANNING;

  toPosition(me, &trackmsg.position1);
  trackmsg.position2 = msg->pos;

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
      PrintDebug(me->ui, "%d Landmark %d %d %d %d", i, node.landmark.type, node.landmark.num1, node.landmark.num2, node.dist);
      if (node.landmark.type == LANDMARK_SWITCH && node.landmark.num1 == BR) {
        PrintDebug(me->ui, "Switch State: %d ", node.num);
      }
    }
  }
}

static int shouldStopNow(Driver* me) {
  int canUseLastSensor =
      ( me->lastSensorBox == me->stopSensorBox &&
        me->lastSensorVal == me->stopSensorVal
      ) || me->useLastSensorNow;

  if (canUseLastSensor) {
    int d = me->distancePassStopSensorToStop - me->distanceFromLastSensor;

    if ((CC++ & 15) == 0) {
      PrintDebug(me->ui, "Navi Nagger. %d", d);
    }
    if (d < 0) {
      // Shit, stopping too late.
      return 2;
    } else if (d < 30) {
      // Stop 20mm early is okay.
      return 1;
    }
  }
  return 0;
}

static void updateStopNode(Driver* me, int speed) {
  // Find the first reverse on the path, stop if possible.
  me->stopNode = me->route.length-2;
  PrintDebug(me->ui, "update stop. %d", me->stopNode);
  for (int i = me->routeRemaining; i < me->route.length-1; i++) {
    if (me->route.nodes[i].num == REVERSE) {
      me->stopNode = i;
      break;
    }
    else if (me->route.nodes[i].landmark.type == LANDMARK_SWITCH &&
        me->route.nodes[i].landmark.num1 == BR) {
      TrackMsg setSwitch;
      setSwitch.type = SET_SWITCH;
      setSwitch.data = me->route.nodes[i].num;
      setSwitch.landmark1 = me->route.nodes[i].landmark;

      PrintDebug(me->ui, "set switch\n");
      Send(me->trackManager, (char*)&setSwitch, sizeof(TrackMsg), (char*)NULL, 0);
    }
  }
  //PrintDebug(me->ui, "calc stopping distance.");

  const int stoppingDistance =
      me->d[speed][(int)me->speedDir][MAX_VAL];
  int stop = stoppingDistance;
  // Find the stopping distance for the stopNode.
  // S------L------L---|-----L---------R------F
  //                   |__stop_dist____|
  // |__travel_dist____|
  // |delay this much..|
  //PrintDebug(me->ui, "Need %d mm at StopNode %d\n", stop, me->stopNode-1);
  for (int i = me->stopNode-1; i >= me->routeRemaining; i--) {
    stop -= me->route.nodes[i].dist;
    //PrintDebug(me->ui, "Stop %d %d\n", stop, i); //5

    if (stop < 0) {
      int previousStop = -stop;
      //PrintDebug(me->ui, "PreviousStop %d\n", previousStop);
      me->stopSensorBox = -1;
      me->stopSensorVal = -1;

      // Find previous sensor.
      for (int j = i; j >= me->routeRemaining; j--) {
        if (me->route.nodes[j].landmark.type == LANDMARK_SENSOR) {
          // The Sensor to begin using distance to next sensor
          me->stopSensorBox = me->route.nodes[j].landmark.num1;
          me->stopSensorVal = me->route.nodes[j].landmark.num2;
          break;
        }
        previousStop += me->route.nodes[j].dist;
      }
      if (me->stopSensorBox == -1) {
        //              |-|---previousStop
        // =========S==*=========F===S
        //          |  |  |_stop_|__stopping distance
        //          |  |___ current position
        //          |______ distanceFromLastSensor
        me->useLastSensorNow = 1;
        previousStop += me->distanceFromLastSensor;
      }
      // Else..
      //     |---stopSensorBox
      // =*==S======S====F===S
      //     |__  |_stop_|____stopping distance
      //       |__|
      //          |______ previous stop
      me->distancePassStopSensorToStop = previousStop;
      PrintDebug(me->ui, "Stop Sensor %d %d\n",
          me->stopSensorBox, me->stopSensorVal);
      PrintDebug(me->ui, "Previous Distance %d \n", previousStop);
      break;
    }
  }
}

static void reRoute(Driver* me, char box, char val) {
}

// Update route traveled as sensors are hit.
static void updateRoute(Driver* me, char box, char val) {
  if (me->routeRemaining == -1) return;

  // See if we triggered
  //PrintDebug(me->ui, "Update round: %d to %d!! \n", me->routeRemaining, me->stopNode);
  for (int i = me->routeRemaining; i < me->stopNode; i++) {
    if (me->route.nodes[i].landmark.type == LANDMARK_SENSOR &&
        me->route.nodes[i].landmark.num1 == box &&
        me->route.nodes[i].landmark.num2 == val)
    {
      PrintDebug(me->ui, "Triggered expected sensor!! %d\n", val);
      me->routeRemaining = i;
      break;
    }
  }

  // TODO if stoppped, update next stopNode.
}

static void dynamicCalibration(Driver* me) {
  if (me->lastSensorUnexpected) return;
  if (me->speed == 0) return; // Cannot calibrate speed zero

  int dTime = me->lastSensorPredictedTime - me->lastSensorActualTime;
  if (dTime > 1000) return;
  int velocity = me->calibrationDistance * 100 * 1000 / (me->lastSensorActualTime - me->calibrationStart);
  int originalVelocity = me->v[(int)me->speed][(int)me->speedDir];
  me->v[(int)me->speed][(int)me->speedDir]
      = (originalVelocity * 8 + velocity * 2) / 10;
}

static void trainSetSpeed(const int speed, const int stopTime, const int delayer, Driver* me) {
  char msg[4];
  msg[1] = (char)me->trainNum;
  if (speed >= 0) {
    if (delayer) {
      //PrintDebug(me->ui, "Reversing speed. cuz its worker %d\n", speed);
      msg[0] = 0xf;
      msg[1] = (char)me->trainNum;
      msg[2] = (char)speed;
      msg[3] = (char)me->trainNum;
      Putstr(com1, msg, 4);

      // Update prediction
      //int action = me->nextSensorVal%2 ? 1 : -1;
      //me->nextSensorVal = me->nextSensorVal + action;
      //action = me->lastSensorVal%2 ? 1 : -1;
      //me->lastSensorVal = me->lastSensorVal + action;
      //int tmp = me->distanceFromLastSensor;
      //me->distanceFromLastSensor = me->distanceToNextSensor;
      //me->distanceToNextSensor = tmp;
      //me->justReversed = 1;
      //Position pos;
      //toPosition(me, &pos);
      // Update prediction

    } else {
      PrintDebug(me->ui, "Set speed. %d %d\n", speed, me->trainNum);
      msg[0] = (char)speed;
      Putstr(com1, msg, 2);
    }
    if (speed > me->speed) {
      me->speedDir = ACCELERATE;
    } else if (speed < me->speed) {
      me->speedDir = DECELERATE;
    }
    me->speed = speed;
  } else {
    PrintDebug(me->ui, "Reverse... %d \n", me->speed);
    DriverMsg delayMsg;
    delayMsg.type = SET_SPEED;
    delayMsg.timestamp = stopTime;
    delayMsg.data2 = (signed char)me->speed;
    //PrintDebug(me->ui, "Using delayer: %d for %d \n", me->delayer, stopTime);

    Reply(me->delayer, (char*)&delayMsg, sizeof(DriverMsg));

    msg[0] = 0;
    msg[1] = (char)me->trainNum;

    Putstr(com1, msg, 2);
    me->speed = 0;
    me->speedDir = DECELERATE;
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
    //PrintDebug(ui, "Delay %d ticks\n", numTick);
    Delay(numTick, timeserver);
    msg.data3 = DELAYER;
  }
}

static void trainNavigateNagger() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = NAVIGATE_NAGGER;
  for (;;) {
    Delay(5, timeserver); // .15 seconds
    msg.timestamp = Time(timeserver) * 10;
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

static void initDriver(Driver* me) {
  char uiName[] = UI_TASK_NAME;
  me->ui = WhoIs(uiName);
  ui = me->ui;
  me->justReversed = 0;

  char trackName[] = TRACK_NAME;
  me->trackManager = WhoIs(trackName);
  me->route.length = 0;
  me->stopCommited = 0; // haven't enabled speed zero yet.
  me->useLastSensorNow = 0;

  char timename[] = TIMESERVER_NAME;
  me->timeserver = WhoIs(timename);

  DriverInitMsg init;
  int controller;
  Receive(&controller, (char*)&init, sizeof(DriverInitMsg));
  Reply(controller, (char*)1, 0);
  me->trainNum = init.trainNum;
  me->uiMsg.nth = init.nth;
  me->uiMsg.type = UPDATE_TRAIN;

  me->speed = 0;
  me->speedDir = ACCELERATE;
  me->distanceToNextSensor = 0;
  me->distanceFromLastSensor = 0;
  me->lastSensorActualTime = 0;

  me->delayer = Create(1, trainDelayer);
  me->sensorWatcher = Create(3, trainSensor);
  me->navigateNagger = Create(2, trainNavigateNagger);
  me->routeRemaining = -1;

  initStoppingDistance((int*)me->d);
  initVelocity((int*)me->v);
}

static void updatePosition(Driver* me, int time){
  if (time) {
    // In mm
    int dPosition = (time - me->reportTime) * getVelocity(me) / 100000;
    me->reportTime = time;
    me->distanceFromLastSensor += dPosition;
    me->distanceToNextSensor -= dPosition;
  }
}

static void sendUiReport(Driver* me) {

  me->uiMsg.velocity = getVelocity(me) / 100;
  if (!me->justReversed){
    me->uiMsg.lastSensorUnexpected = me->lastSensorUnexpected;
    me->uiMsg.lastSensorBox            = me->lastSensorBox;
    me->uiMsg.lastSensorVal            = me->lastSensorVal;
    me->uiMsg.lastSensorActualTime     = me->lastSensorActualTime;
    me->uiMsg.lastSensorPredictedTime  = me->lastSensorPredictedTime;
  } else {
    me->uiMsg.lastSensorUnexpected     = 0;
    me->uiMsg.lastSensorBox            = 0;
    me->uiMsg.lastSensorVal            = 0;
    me->uiMsg.lastSensorActualTime     = 0;
    me->uiMsg.lastSensorPredictedTime  = 0;
  }

  me->uiMsg.speed                    = me->speed;      // 0 - 14
  me->uiMsg.speedDir                 = me->speedDir;
  me->uiMsg.distanceFromLastSensor   = me->distanceFromLastSensor;
  me->uiMsg.distanceToNextSensor     = me->distanceToNextSensor;

  me->uiMsg.nextSensorBox            = me->nextSensorBox;
  me->uiMsg.nextSensorVal            = me->nextSensorVal;
  me->uiMsg.nextSensorPredictedTime  = me->nextSensorPredictedTime;

  Send(me->ui, (char*)&(me->uiMsg), sizeof(TrainUiMsg), (char*)1, 0);
}

static void updatePredication(Driver* me) {
  int now = Time(me->timeserver) * 10;
  TrackNextSensorMsg tMsg;
  TrackMsg qMsg;
  qMsg.type = QUERY_NEXT_SENSOR_FROM_POS;
  toPosition(me, &qMsg.position1);
  Send(me->trackManager, (char*)&qMsg, sizeof(TrackMsg),
        (char*)&tMsg, sizeof(TrackNextSensorMsg));

  me->distanceToNextSensor = tMsg.dist;
  me->nextSensorBox = tMsg.sensor.num1;
  me->nextSensorVal = tMsg.sensor.num2;
  me->nextSensorPredictedTime =
    now + me->distanceToNextSensor*100000 / getVelocity(me) - 50; // 50 ms delay for sensor query.

  sendUiReport(me);
}

static void driver() {
  Driver me;
  initDriver(&me);

  // used to store one set_route msg when train's current position is unknown
  int hasTempRouteMsg = 0;
  unsigned int naggCount = 0;
  DriverMsg tempRouteMsg;

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
        Reply(replyTid, (char*)&me.speed, 4);
        break;
      }
      case SET_SPEED: {
        trainSetSpeed(msg.data2,
                      getStoppingTime(&me),
                      (msg.data3 == DELAYER),
                      &me);
        if (msg.data3 != DELAYER) {
          //PrintDebug(me.ui, "Replied to %d\n", replyTid);
          Reply(replyTid, (char*)1, 0);
          sendUiReport(&me);
          break;
        } else if (me.route.length != 1) {
          // Delayer came back. Reverse command completed
          me.stopCommited = 0; // We're moving again.
          // We've completed everything up to the reverse node.
          me.routeRemaining = me.stopNode+1;
          // Calculate the next stop node.
          updateStopNode(&me, msg.data2);
          // Update
        }
      }
      case DELAYER: {
        PrintDebug(me.ui, "delayer come back.");
        break;
      }
      case SENSOR_TRIGGER: {
        me.justReversed = 0;
        if (msg.data2 != me.nextSensorBox || msg.data3 != me.nextSensorVal) {
          me.lastSensorUnexpected = 1;
          //reRoute(&me, msg.data2, msg.data3); // TODO
        } else {
          me.lastSensorUnexpected = 0;
        }
        updateRoute(&me, msg.data2, msg.data3);
        me.lastSensorBox = msg.data2; // Box
        me.lastSensorVal = msg.data3; // Val
        me.lastSensorActualTime = msg.timestamp;
        dynamicCalibration(&me);
        me.lastSensorPredictedTime = me.nextSensorPredictedTime;

        TrackNextSensorMsg tMsg;
        TrackMsg qMsg;
        qMsg.type = QUERY_NEXT_SENSOR_FROM_SENSOR;
        qMsg.landmark1.type = LANDMARK_SENSOR;
        qMsg.landmark1.num1 = me.lastSensorBox;
        qMsg.landmark1.num2 = me.lastSensorVal;
        Send(me.trackManager, (char*)&qMsg, sizeof(TrackMsg),
              (char*)&tMsg, sizeof(TrackNextSensorMsg));
        me.calibrationStart = msg.timestamp;
        me.calibrationDistance = tMsg.dist;
        me.distanceFromLastSensor = 0;
        me.distanceToNextSensor = tMsg.dist;
        me.reportTime = msg.timestamp;
        me.nextSensorBox = tMsg.sensor.num1;
        me.nextSensorVal = tMsg.sensor.num2;
        me.nextSensorPredictedTime =
          msg.timestamp + me.distanceToNextSensor*100000 / getVelocity(&me) - 50; // 50 ms delay for sensor query.

        updatePosition(&me, msg.timestamp);
        sendUiReport(&me);
        if (hasTempRouteMsg) {
          getRoute(&me, &tempRouteMsg);
          updateStopNode(&me, tempRouteMsg.data2);
          trainSetSpeed(tempRouteMsg.data2, 0, 0, &me);
          hasTempRouteMsg = 0;
        }
        break;
      }
      case NAVIGATE_NAGGER: {
        updatePosition(&me, msg.timestamp);
        if (me.routeRemaining != -1) {
          if (!me.stopCommited) {
            if (shouldStopNow(&me)) {
              if (me.route.nodes[me.stopNode].num == REVERSE) {
                PrintDebug(me.ui, "Navi reversing.");
                const int speed = -1;
                trainSetSpeed(speed, getStoppingTime(&me), 0, &me);
              }
              else {
                PrintDebug(me.ui, "Navi Nagger stopping.");
                const int speed = 0;  // Set speed zero.
                trainSetSpeed(speed, getStoppingTime(&me), 0, &me);
                me.route.length = 0; // Finished the route.
              }
              me.stopCommited = 1;
              me.useLastSensorNow = 0;
            }
          }
        }
        naggCount++;
        naggCount = (naggCount & 3);
        if (naggCount == 0){
          sendUiReport(&me);
        }
        break;
      }
      case SET_ROUTE: {
        Reply(replyTid, (char*)1, 0);
        me.stopCommited = 0;
        if (me.lastSensorActualTime > 0) {
          getRoute(&me, &msg);
          updateStopNode(&me, msg.data2);
          trainSetSpeed(msg.data2, 0, 0, &me);
        } else {
          trainSetSpeed(5, 0, 0, &me);
          hasTempRouteMsg = 1;
          tempRouteMsg = msg;
        }
        break;
      }
      case BROADCAST_UPDATE_PREDICATION: {
        updatePredication(&me);
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
  com1 = WhoIs(com1Name);

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

    if (msg.trainNum == 255) {
      // Broadcast, can't receive replies
      Reply(tid, (char*)1, 0);
      for (int i = 0; i < 80; i++) {
        if (trainTid[i] != -1) {
          Send(trainTid[i], (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
        }
      }
    } else {
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
}

int startDriverControllerTask() {
  return Create(5, trainController);
}
