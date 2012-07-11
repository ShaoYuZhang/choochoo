#include "Driver.h"
#include <IoServer.h>
#include <NameServer.h>
#include <ts7200.h>
#include <util.h>
#include <TimeServer.h>
#include <IoHelper.h>
#include <syscall.h>
#include <CalibrationData.h>
#include <Sensor.h>
#include <Track.h>

static void trainSetSpeed(const int speed, const int stopTime, const int delayer, Driver* me);
static void printLandmark(Driver* me, TrackLandmark* l);
static void trainDelayer();
static void trainSensor();
static void trainNavigateNagger();
static void printRoute(Driver* me);
static void sendUiReport(Driver* me);
static void QueryNextSensor(Driver* me, TrackNextSensorMsg* trackMsg);
static int QueryIsSensorReserved(Driver* me, int box, int val);
static void setRoute(Driver* me, DriverMsg* msg);

static int getStoppingDistance(Driver* me) {
  return me->d[(int)me->speed][(int)me->speedDir][MAX_VAL];
}

// mm/s
static int getVelocity(Driver* me, int speed, int dir){
  return me->v[speed][dir];
  if (me->isAding) {
    int now = Time(me->timeserver) * 10;
    return eval_velo(&(me->adPoly), now);
  } else {
    return me->v[(int)me->speed][(int)me->speedDir];
  }
}

static int getStoppingTime(Driver* me) {
  return 2* getStoppingDistance(me) * 100000 /
    getVelocity(me, me->speed, me->speedDir);
}

static void toPosition(Driver* me, Position* pos) {
  pos->landmark1.type = me->lastSensorIsTerminal ? LANDMARK_END : LANDMARK_SENSOR;
  pos->landmark1.num1 = me->lastSensorBox;
  pos->landmark1.num2 = me->lastSensorVal;
  pos->landmark2.type = me->nextSensorIsTerminal ? LANDMARK_END : LANDMARK_SENSOR;
  pos->landmark2.num1 = me->nextSensorBox;
  pos->landmark2.num2 = me->nextSensorVal;
  pos->offset = (int)me->distanceFromLastSensor;
}

static int interpolateStoppingDistance(Driver* me, int velocity) {
  int speed = 14;
  float percentUp = -1.0;
  float percentDown = -1.0;

  for (int i = 1; i < 15; i++) {
    if (me->v[i][MAX_VAL] >= velocity && me->v[i-1][MAX_VAL] <= velocity) {
      speed = i;
      float diff = (float)(me->v[i][MAX_VAL] - me->v[i-1][MAX_VAL]);
      percentUp = (float)(me->v[i][MAX_VAL] - velocity) / diff;
      percentDown = 1.0 - percentUp;
      break;
    }
  }

  //TrainDebug(me, "Sp: %d V:%d Up:%d Down:%d", speed, velocity,
  //    (int)(percentUp*100), (int)(percentDown*100));

  float stopUp = (float)me->d[speed][me->speedDir][MAX_VAL];
  float stopDown = (float)me->d[speed-1][me->speedDir][MAX_VAL];

  return stopUp * percentUp + stopDown * percentDown;
}

static void trySetSwitch_and_getNextSwitch(Driver* me) {
  TrackMsg setSwitch;
  setSwitch.type = SET_SWITCH;
  setSwitch.trainNum = me->trainNum;
  setSwitch.data = me->route.nodes[me->nextSetSwitchNode].num;
  setSwitch.landmark1 = me->route.nodes[me->nextSetSwitchNode].landmark;

  char reply = SET_SWITCH_FAIL;
  Send(me->trackManager, (char*)&setSwitch, sizeof(TrackMsg), &reply, 1);

  if (reply == SET_SWITCH_SUCCESS) {
    TrainDebug(me, "Set Switch Success");
    printLandmark(me, &setSwitch.landmark1);
    for (int i = me->routeRemaining; i < me->route.length-1; i++) {
      if (me->route.nodes[i].landmark.type == LANDMARK_SWITCH &&
          me->route.nodes[i].landmark.num1 == BR && me->nextSetSwitchNode != -1) {
        me->nextSetSwitchNode = i;
      }
    }
  }
}

static int reserveMoreTrack(Driver* me, TrackNextSensorMsg* trackMsg) {
  ReleaseOldAndReserveNewTrackMsg qMsg;
  qMsg.type = RELEASE_OLD_N_RESERVE_NEW;
  qMsg.trainNum = me->trainNum;
  qMsg.stoppingDistance = interpolateStoppingDistance(
      me, getVelocity(me, me->speed, me->speedDir));
  qMsg.lastSensor.type = LANDMARK_SENSOR;
  qMsg.lastSensor.num1 = me->lastSensorBox;
  qMsg.lastSensor.num2 = me->lastSensorVal;

  if (!me->positionFinding) {
    if (trackMsg->numPred > 10) {
      TrainDebug(me, "__Can't reserve >10 predictions");
    } else {
      TrainDebug(me, "Reserving track");
      qMsg.numPredSensor = trackMsg->numPred;
      for(int i = 0; i < trackMsg->numPred; i++) {
        qMsg.predSensor[i] = trackMsg->predictions[i].sensor;
        printLandmark(me, &qMsg.predSensor[i]);
      }
    }
  } else {
    qMsg.numPredSensor = 0;
  }

  char status = RESERVE_FAIL;
  Send(me->trackManager, (char*)&qMsg, sizeof(ReleaseOldAndReserveNewTrackMsg),
      &status, 1);

  return status;
}

static void updatePrediction(Driver* me) {
  int now = Time(me->timeserver) * 10;
  TrackNextSensorMsg trackMsg;
  TrackMsg qMsg;
  qMsg.type = QUERY_NEXT_SENSOR_FROM_POS;
  toPosition(me, &qMsg.position1);
  Send(me->trackManager, (char*)&qMsg, sizeof(TrackMsg),
        (char*)&trackMsg, sizeof(TrackNextSensorMsg));

  for (int i = 0; i < trackMsg.numPred; i++) {
    me->predictions[i] = trackMsg.predictions[i];
  }
  me->numPredictions = trackMsg.numPred;

  TrackSensorPrediction primaryPrediction = trackMsg.predictions[0];
  me->distanceToNextSensor = primaryPrediction.dist;
  if (primaryPrediction.sensor.type != LANDMARK_SENSOR &&
      primaryPrediction.sensor.type != LANDMARK_END) {
    TrainDebug(me, "QUERY_NEXT_SENSOR_FROM_SENSOR ..bad");
  }
  me->nextSensorIsTerminal = (primaryPrediction.sensor.type == LANDMARK_END);

  if (primaryPrediction.sensor.num2 != 0) {
    me->nextSensorBox = primaryPrediction.sensor.num1;
    me->nextSensorVal = primaryPrediction.sensor.num2;
    me->nextSensorPredictedTime =
      now + me->distanceToNextSensor*100000 /
      getVelocity(me, me->speed, me->speedDir) - 50; // 50 ms delay for sensor query.
  } else {
    TrainDebug(me, "No prediction.. has position");
    printLandmark(me, &qMsg.position1.landmark1);
    printLandmark(me, &qMsg.position1.landmark2);
    TrainDebug(me, "Offset %d", qMsg.position1.offset);
  }
  sendUiReport(me);
}

static void getRoute(Driver* me, DriverMsg* msg) {
  TrainDebug(me, "Getting Route.");
  TrackMsg trackmsg;
  trackmsg.type = ROUTE_PLANNING;

  toPosition(me, &trackmsg.position1);
  printLandmark(me, &trackmsg.position1.landmark1);
  printLandmark(me, &trackmsg.position1.landmark2);
  TrainDebug(me, "Offset %d", trackmsg.position1.offset);

  trackmsg.position2 = msg->pos;
  trackmsg.data = (char)me->trainNum;

  Send(me->trackManager, (char*)&trackmsg,
      sizeof(TrackMsg), (char*)&(me->route), sizeof(Route));
  me->routeRemaining = 0;
  printRoute(me);
}

static int shouldStopNow(Driver* me) {
  if (me->stopNow) {
    return 2; // no room to stop, must stop now
  }
  if ( me->lastSensorBox == me->stopSensorBox &&
        me->lastSensorVal == me->stopSensorVal) {
    me->stopSensorHit = 1;
  }
  int canUseLastSensor =
      ( me->lastSensorBox == me->stopSensorBox &&
        me->lastSensorVal == me->stopSensorVal
      ) || me->useLastSensorNow;

  if (canUseLastSensor) {
    int d = me->distancePassStopSensorToStop - (int)me->distanceFromLastSensor;

    me->CC &= 15;
    if (me->CC++ == 0) {
      TrainDebug(me, "Navi Nagger. %d", d);
    }
    if (d < 0) {
      // Shit, stopping too late.
      return 2;
    } else if (d < 20) {
      // Stop 20mm early is okay.
      return 1;
    }
  }

  if (!canUseLastSensor && me->stopSensorHit) {
    return 2; // Missed a sensor stop now.
  }
  return 0;
}

static void updateStopNode(Driver* me, int speed) {
  // Find the first reverse on the path, stop if possible.
  me->stopNode = me->route.length-1;
  me->nextSetSwitchNode = -1;
  const int stoppingDistance =
    interpolateStoppingDistance(me,
        me->v[speed][me->speedDir]);
        //getVelocity(me, speed, me->speedDir));
  int stop = stoppingDistance;
  for (int i = me->routeRemaining; i < me->route.length-1; i++) {
    if (me->route.nodes[i].num == REVERSE) {
      me->stopNode = i;
      break;
    }
    else if (me->route.nodes[i].landmark.type == LANDMARK_SWITCH &&
        me->route.nodes[i].landmark.num1 == BR && me->nextSetSwitchNode == -1) {
      TrainDebug(me, "Will try to set:");
      printLandmark(me, &(me->route.nodes[i].landmark));
      me->nextSetSwitchNode = i;
    }
  }
  TrainDebug(me, "UpdateStop. Node:%d %dmm Spd:%d Dir:%d",
      me->stopNode, stoppingDistance, speed, me->speedDir);

  // Find the stopping distance for the stopNode.
  // S------L------L---|-----L---------R------F
  //                   |__stop_dist____|
  // |__travel_dist____|
  // |delay this much..|
  //TrainDebug(me, "Need %d mm at StopNode %d", stop, me->stopNode-1);
  for (int i = me->stopNode-1; i >= me->routeRemaining; i--) {
    // Minus afterwards so that stoppping distance can be zero.
    if (stop > 0) {
      stop -= me->route.nodes[i].dist;
      TrainDebug(me, "Stop %d %d", stop, i);
    }

    if (stop <= 0) {
      int previousStop = -stop;
      //TrainDebug(me, "PreviousStop %d", previousStop);
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
        previousStop += me->route.nodes[j-1].dist; // add distance from last node to this node
      }
      if (me->stopSensorBox == -1) {
        //              |-|---previousStop
        // =========S==*=========F===S
        //          |  |  |_stop_|__stopping distance
        //          |  |___ current position
        //          |______ distanceFromLastSensor
        me->useLastSensorNow = 1;
        previousStop += (int)me->distanceFromLastSensor;
      }
      // Else..
      //     |---stopSensorBox
      // =*==S======S====F===S
      //     |__  |_stop_|____stopping distance
      //       |__|
      //          |______ previous stop
      me->distancePassStopSensorToStop = previousStop;
      TrainDebug(me, "Stop Sensor %c%d",
          'A'+me->stopSensorBox, me->stopSensorVal);
      TrainDebug(me, "Stop %dmm after sensor", previousStop);
      break;
    }
  }
  if (stop > 0) {
      TrainDebug(me, "No room to stop??? %d", stop);
      TrainDebug(me, "StopNode-1 %d, remaining %d ", me->stopNode-1,
          me->routeRemaining);
      me->stopNow = 1;
  }

  TrainDebug(me, "Finish update stop %d %d %d", me->stopNode,
  (me->stopNode == (me->route.length-1)), (me->route.nodes[me->stopNode].num == REVERSE));
  int stopAfterReverse =
      ((me->stopNode == (me->route.length-2)) &&
       (me->route.nodes[me->stopNode].num == REVERSE));
  me->speedAfterReverse = stopAfterReverse ? 0 : -1;
}

// Update route traveled as sensors are hit.
static void updateRoute(Driver* me, char box, char val) {
  if (me->routeRemaining == -1) return;

  // See if we triggered a sensor on the route.
  for (int i = me->routeRemaining; i < me->stopNode; i++) {
    if (me->route.nodes[i].landmark.type == LANDMARK_SENSOR &&
        me->route.nodes[i].landmark.num1 == box &&
        me->route.nodes[i].landmark.num2 == val)
    {
      TrainDebug(me, "Triggered expected sensor!! %d", val);
      me->routeRemaining = i;
      break;
    }
  }

  // TODO if stoppped, update next stopNode.
}

static void dynamicCalibration(Driver* me) {
  if (me->isAding) return; // TODO doesn't cover all cases
  if (me->lastSensorUnexpected) return;
  if (me->speed == 0) return; // Cannot calibrate speed zero

  int dTime = me->lastSensorPredictedTime - me->lastSensorActualTime;
  if (dTime > 1000) return;
  if (dTime < -1000) return;
  int velocity = me->calibrationDistance * 100 * 1000 / (me->lastSensorActualTime - me->calibrationStart);
  int originalVelocity = me->v[(int)me->speed][(int)me->speedDir];
  me->v[(int)me->speed][(int)me->speedDir]
      = (originalVelocity * 85 + velocity * 15) / 100;
}

static void trainSetSpeed(const int speed, const int stopTime, const int delayer, Driver* me) {
  char msg[4];
  msg[1] = (char)me->trainNum;

  if (me->lastSensorActualTime > 0) {
    // a/d related stuff
    int newSpeed = speed >=0 ? speed : 0;
    int now = Time(me->timeserver) * 10;
    if (me->speed == newSpeed) {
      // do nothing
    }
    else if (me->speed == 0) {
      // accelerating from 0
      int v0 = getVelocity(me, me->speed, me->speedDir);
      int v1 = me->v[newSpeed][ACCELERATE];
      int t0 = now + 8; // compensate for time it takes to send to train
      int t1 = now + 8 + me->a[newSpeed];
      poly_init(&me->adPoly, t0, t1, v0, v1);
      me->isAding = 1;
      me->lastReportDist = 0;
      me->adEndTime = t1;
    } else if (newSpeed == 0) {
      // decelerating to 0
      int v0 = getVelocity(me, me->speed, me->speedDir);
      int v1 = me->v[newSpeed][DECELERATE];
      int t0 = now + 8; // compensate for time it takes to send to train
      int t1 = now + 8 + getStoppingTime(me);
      poly_init(&me->adPoly, t0, t1, v0, v1);
      me->isAding = 1;
      me->lastReportDist = 0;
      me->adEndTime = t1;
    }
  }

  TrainDebug(me, "Train Setting Speed %d", speed);

  if (speed >= 0) {
    if (delayer) {
      TrainDebug(me, "Reversing speed.------- %d", speed);
      msg[0] = 0xf;
      msg[1] = (char)me->trainNum;
      msg[2] = (char)speed;
      msg[3] = (char)me->trainNum;
      Putstr(me->com1, msg, 4);

      TrainDebug(me, "Next Sensor: %d %d", me->nextSensorIsTerminal, me->lastSensorIsTerminal);
      // Update prediction
      if (me->nextSensorIsTerminal) {
        me->nextSensorBox = me->nextSensorBox == EX ? EN : EX;

        TrainDebug(me, "LAst Sensor: %d ", me->lastSensorVal);
      } else {
        int action = me->nextSensorVal%2 == 1 ? 1 : -1;
        me->nextSensorVal = me->nextSensorVal + action;
      }
      if (me->lastSensorIsTerminal) {
        me->lastSensorBox = me->lastSensorBox == EX ? EN : EX;
      } else {
        int action = me->lastSensorVal%2 == 1 ? 1 : -1;
        me->lastSensorVal = me->lastSensorVal + action;
      }

      float distTemp = me->distanceFromLastSensor;
      me->distanceFromLastSensor = me->distanceToNextSensor;
      me->distanceToNextSensor = distTemp;

      char valTemp = me->nextSensorVal;
      me->nextSensorVal = me->lastSensorVal;
      me->lastSensorVal = valTemp;

      char boxTemp = me->nextSensorBox;
      me->nextSensorBox = me->lastSensorBox;
      me->lastSensorBox = boxTemp;

      if (me->nextSensorIsTerminal || me->lastSensorIsTerminal){
        char isTemp = me->nextSensorIsTerminal;
        me->nextSensorIsTerminal = me->lastSensorIsTerminal;
        me->lastSensorIsTerminal = isTemp;
      }

      // Reserve more
      TrackNextSensorMsg trackMsg;
      QueryNextSensor(me, &trackMsg);
      // Reserve the track above train and future (covers case of init)
      int reserveStatus = reserveMoreTrack(me, &trackMsg);
      if  (reserveStatus == RESERVE_FAIL) {
        trainSetSpeed(0, 0, 0, me);
        me->rerouteCountdown = 200; // wait 2 seconds then reroute.
      }

      // Update prediction
      updatePrediction(me);
    } else {
      TrainDebug(me, "Set speed. %d %d", speed, me->trainNum);
      msg[0] = (char)speed;
      Putstr(me->com1, msg, 2);
    }
    if (speed > me->speed) {
      me->speedDir = ACCELERATE;
    } else if (speed < me->speed) {
      me->speedDir = DECELERATE;
    }
    me->speed = speed;
  } else {
    TrainDebug(me, "Reverse... %d ", me->speed);
    DriverMsg delayMsg;
    delayMsg.type = SET_SPEED;
    delayMsg.timestamp = stopTime;
    if (me->speedAfterReverse == -1) {
      delayMsg.data2 = (signed char)me->speed;
    } else {
      delayMsg.data2 = (signed char)me->speedAfterReverse;
    }
    TrainDebug(me, "Using delayer: %d for %d", me->delayer, stopTime);

    Reply(me->delayer, (char*)&delayMsg, sizeof(DriverMsg));

    msg[0] = 0;
    msg[1] = (char)me->trainNum;

    Putstr(me->com1, msg, 2);
    me->speed = 0;
    me->speedDir = DECELERATE;
  }
}

static void initDriver(Driver* me) {
  char uiName[] = UI_TASK_NAME;
  me->ui = WhoIs(uiName);
  me->CC = 0;
  me->speedAfterReverse = -1;
  me->rerouteCountdown = -1;
  me->nextSetSwitchNode = -1;

  char trackName[] = TRACK_NAME;
  me->trackManager = WhoIs(trackName);
  me->route.length = 0;
  me->stopCommited = 0; // haven't enabled speed zero yet.
  me->useLastSensorNow = 0;
  me->stopNow = 0;
  me->positionFinding = 0;
  me->stopSensorHit = 0;
  me->nextSensorIsTerminal = 0;
  me->lastSensorIsTerminal = 0;
  me->lastSensorVal = 0; // NOte to ui to don't print sensor.
  me->setSwitchNaggerCount = 0;

  char timename[] = TIMESERVER_NAME;
  me->timeserver = WhoIs(timename);

  DriverInitMsg init;
  Receive(&(me->trainController), (char*)&init, sizeof(DriverInitMsg));
  Reply(me->trainController, (char*)1, 0);
  me->trainNum = init.trainNum;
  me->uiMsg.nth = init.nth;
  me->uiMsg.trainNum = (char)init.trainNum;
  me->com1 = init.com1;
  me->uiMsg.type = UPDATE_TRAIN;

  me->speed = 0;
  me->speedDir = ACCELERATE;
  me->distanceToNextSensor = 0;
  me->distanceFromLastSensor = 0;
  me->lastSensorActualTime = 0;
  me->lastSensorDistanceError = 0;

  me->delayer = Create(1, trainDelayer);
  me->sensorWatcher = Create(3, trainSensor);
  me->navigateNagger = Create(2, trainNavigateNagger);
  me->routeRemaining = -1;

  me->isAding = 0;

  initStoppingDistance((int*)me->d);
  initVelocity((int*)me->v);
  initAccelerationProfile((int*)me->a);
}

static void updatePosition(Driver* me, int time){
  if (time) {
    float dPosition;
    if (me->isAding && time > me->adPoly.t0) {
      float dist;
      if (time > me->adEndTime) {
        me->isAding = 0;
        dist = eval_dist(&me->adPoly, me->adEndTime);
      } else {
        dist = eval_dist(&me->adPoly, time);
      }
      dPosition = dist - me->lastReportDist;
      if (dPosition < 0) {
        dPosition = 0;
      }
      me->lastReportDist = dist;

    } else {
      // In mm
      dPosition = (time - me->lastPosUpdateTime) * getVelocity(me, me->speed, me->speedDir) / 100000.0;
    }
    me->lastPosUpdateTime = time;
    me->distanceFromLastSensor += dPosition;
    me->distanceToNextSensor -= dPosition;

    //haha &= 15;
    //if (haha++ == 0 && 0) {
    //  if (me->isAding && time > me->adPoly.t0) {
    //    TrainDebug(me, "AD UpdatePos: %d %d",
    //        (int)me->distanceToNextSensor, (int)me->distanceToNextSensor);
    //  }
    //  else {
    //    TrainDebug(me, "VE UpdatePos: %d %d",
    //        (int)me->distanceToNextSensor, (int)me->distanceToNextSensor);
    //  }
    //}
  }
}

void driver() {
  Driver me;
  initDriver(&me);

  unsigned int naggCount = 0;

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
          //TrainDebug(&me, "Replied to %d", replyTid);
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
          // if the reverse is last node, nothing to do
          // if it isn't.. it should speed up again.
        }
      }
      case DELAYER: {
        TrainDebug(&me, "delayer come back.");
        break;
      }
      case SENSOR_TRIGGER: {
        me.lastSensorIsTerminal = 0;

        // only handle sensor reports in primary + secondary prediction if not position finding
        int sensorReportValid = 0;
        TrackLandmark conditionLandmark;
        int condition;
        int isSensorReserved = QueryIsSensorReserved(&me, msg.data2, msg.data3);
        if (me.positionFinding) {
          sensorReportValid = 1;
          me.lastSensorUnexpected = 1;
        } else if (isSensorReserved) {
          for (int i = 0; i < me.numPredictions; i ++) {
            TrackLandmark predictedSensor = me.predictions[i].sensor;
            if (predictedSensor.type == LANDMARK_SENSOR && predictedSensor.num1 == msg.data2 && predictedSensor.num2 == msg.data3) {
              sensorReportValid = 1;
              if (i != 0) {
                // secondary prediction, need to do something about them
                conditionLandmark = me.predictions[i].conditionLandmark;
                condition = me.predictions[i].condition;
                me.lastSensorUnexpected = 1;
                // Stop and then try to reroute.
                trainSetSpeed(0, 0, 0, &me);
                me.rerouteCountdown = 200; // wait 2 seconds then reroute.
              } else {
                me.lastSensorUnexpected = 0;
              }
            }
          }
        }
        if (sensorReportValid) {
          updateRoute(&me, msg.data2, msg.data3);
          me.lastSensorBox = msg.data2; // Box
          me.lastSensorVal = msg.data3; // Val
          me.lastSensorActualTime = msg.timestamp;
          dynamicCalibration(&me);
          me.lastSensorPredictedTime = me.nextSensorPredictedTime;

          TrackNextSensorMsg trackMsg;
          QueryNextSensor(&me, &trackMsg);
          // Reserve the track above train and future (covers case of init)
          int reserveStatus = reserveMoreTrack(&me, &trackMsg);
          if  (reserveStatus == RESERVE_FAIL) {
            trainSetSpeed(0, 0, 0, &me);
            me.rerouteCountdown = 200; // wait 2 seconds then reroute.
          }

          for (int i = 0; i < trackMsg.numPred; i++) {
            me.predictions[i] = trackMsg.predictions[i];
          }
          me.numPredictions = trackMsg.numPred;

          TrackSensorPrediction primaryPrediction = me.predictions[0];
          me.calibrationStart = msg.timestamp;
          me.calibrationDistance = primaryPrediction.dist;
          int dPos = 50 * getVelocity(&me, me.speed, me.speedDir) / 100000.0;
          me.lastSensorDistanceError =  -(int)me.distanceToNextSensor - dPos;
          me.distanceFromLastSensor = dPos;
          me.distanceToNextSensor = primaryPrediction.dist - dPos;
          me.lastPosUpdateTime = msg.timestamp;
          if (primaryPrediction.sensor.type != LANDMARK_SENSOR &&
              primaryPrediction.sensor.type != LANDMARK_END) {
            TrainDebug(&me, "QUERY_NEXT_SENSOR_FROM_SENSOR ..bad");
          }
          me.nextSensorIsTerminal = (primaryPrediction.sensor.type == LANDMARK_END);
          me.nextSensorBox = primaryPrediction.sensor.num1;
          me.nextSensorVal = primaryPrediction.sensor.num2;
          me.nextSensorPredictedTime =
            msg.timestamp + me.distanceToNextSensor*100000 /
            getVelocity(&me, me.speed, me.speedDir);

          updatePosition(&me, msg.timestamp);
          sendUiReport(&me);
          if (me.positionFinding) {
            trainSetSpeed(0, 0, 0, &me); // Found position, stop.
            me.positionFinding = 0;
            FinishPositionFinding(me.trainNum, me.trainController);
          }
        }
        break;
      }
      case NAVIGATE_NAGGER: {
        updatePosition(&me, msg.timestamp);
        if (me.routeRemaining != -1) {
          if (!me.stopCommited) {
            if (shouldStopNow(&me)) {
              if (me.route.nodes[me.stopNode].num == REVERSE) {
                TrainDebug(&me, "Navi reversing.");
                const int speed = -1;
                trainSetSpeed(speed, getStoppingTime(&me), 0, &me);
              }
              else {
                TrainDebug(&me, "Navi Nagger stopping.");
                const int speed = 0;  // Set speed zero.
                trainSetSpeed(speed, getStoppingTime(&me), 0, &me);
                me.route.length = 0; // Finished the route.
              }
              me.stopCommited = 1;
              me.useLastSensorNow = 0;
              me.stopNow = 0;
              me.stopSensorHit = 0;
            }
          }
        }

        if (me.nextSetSwitchNode != -1 && (++me.setSwitchNaggerCount & 3) == 0) {
          trySetSwitch_and_getNextSwitch(&me);
        }
        if (me.rerouteCountdown-- == 0) setRoute(&me, &(me.routeMsg));
        if ((++naggCount & 15) == 0) sendUiReport(&me);
        break;
      }
      case SET_ROUTE: {
        Reply(replyTid, (char*)1, 0);
        me.routeMsg = msg;
        setRoute(&me, &msg);
        break;
      }
      case BROADCAST_UPDATE_PREDICTION: {
        updatePrediction(&me);
        break;
      }
      case FIND_POSITION: {
        trainSetSpeed(5, 0, 0, &me);
        me.positionFinding = 1;
        break;
      }
      default: {
        ASSERT(FALSE, "Not suppported train message type.");
      }
    }
  }
}

#include <DriverHelper.c>
