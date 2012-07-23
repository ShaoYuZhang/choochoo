#include <Driver.h>
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
static void trainStopDelayer();
static void trainSensor();
static void trainNavigateNagger();
static void printRoute(Driver* me);
static void sendUiReport(Driver* me);
static void QueryNextSensor(Driver* me, TrackNextSensorMsg* trackMsg);
static int QueryIsSensorReserved(Driver* me, int box, int val);
static void setRoute(Driver* me, DriverMsg* msg);
static void updatePrediction(Driver* me);
static int reserveMoreTrack(Driver* me, int stopped, int stoppingDistance);
static void toPosition(Driver* me, Position* pos);
static void trySetSwitch_and_getNextSwitch(Driver* me);
static void updatePrediction(Driver* me);
static void getRoute(Driver* me, DriverMsg* msg);
static int shouldStopNow(Driver* me);
static void updateStopNode(Driver* me);
static void updateRoute(Driver* me, char box, char val);
static void updateSetSwitch(Driver* me);

static int getStoppingDistance(Driver* me) {
  return me->d[(int)me->speed][(int)me->speedDir][MAX_VAL];
}

// mm/s
static int getVelocity(Driver* me){
  if (me->isAding) {
    int now = Time(me->timeserver) * 10;
    return eval_velo(&(me->adPoly), now);
  } else {
    return me->v[(int)me->speed][(int)me->speedDir];
  }
}

static int isLost(Driver* me) {
  if (me->currentlyLost != 1 &&
      me->positionFinding == 0 &&
      me->distanceFromLastSensor > me->distanceToLongestSecondary+100) {
    TrainDebug(me, "I'm Lost... ");
    //me->positionFinding = 1;
    //me->routeRemaining = -1; // No more route following
    //BroadcastLost(me->trainController);
    me->currentlyLost = 1;
    return 0;
  }
  return 1;
}

static int interpolateStoppingDistance(Driver* me, int velocity) {
  int speed = 14;
  float percentUp = -1.0;
  float percentDown = -1.0;

  for (int i = 1; i < 15; i++) {
    if (me->v[i][MAX_VAL] >= velocity && me->v[i-1][MAX_VAL] <= velocity) {
      speed = i;
      float diff = (float)(me->v[i][MAX_VAL] - me->v[i-1][MAX_VAL]);
      percentDown = (float)(me->v[i][MAX_VAL] - velocity) / diff;
      percentUp = 1.0 - percentDown;
      break;
    }
  }

  //TrainDebug(me, "Sp: %d V:%d Up:%d Down:%d", speed, velocity,
  //    (int)(percentUp*100), (int)(percentDown*100));

  float stopUp = (float)me->d[speed][me->speedDir][MAX_VAL];
  float stopDown = (float)me->d[speed-1][me->speedDir][MAX_VAL];

  return stopUp * percentUp + stopDown * percentDown;
}

static int getStoppingTime(Driver* me) {
  return 2* interpolateStoppingDistance(me, getVelocity(me)) * 100000 /
    getVelocity(me);
}

static void reroute(Driver* me) {
  trainSetSpeed(0, getStoppingTime(me), 0, me);
  me->rerouteCountdown = GET_TIMER4() % 2 == 0 ? 45 : 55; // wait ~1 seconds then reroute.
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
      int v0 = getVelocity(me);
      int v1 = me->v[newSpeed][ACCELERATE];
      int t0 = now + 8; // compensate for time it takes to send to train
      int t1 = now + 8 + me->a[newSpeed];
      poly_init(&me->adPoly, t0, t1, v0, v1);
      me->isAding = 1;
      me->lastReportDist = 0;
      me->adEndTime = t1;
    } else if (newSpeed == 0) {
      // decelerating to 0
      int v0 = getVelocity(me);
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

      //TrainDebug(me, "Next Sensor: %d %d", me->nextSensorIsTerminal, me->lastSensorIsTerminal);
      // Update prediction
      if (me->nextSensorIsTerminal) {
        me->nextSensorBox = me->nextSensorBox == EX ? EN : EX;

        //TrainDebug(me, "LAst Sensor: %d ", me->lastSensorVal);
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
      // Reserve the track above train and future (covers case of init)

      // Update prediction
      updatePrediction(me);
      int reserveStatus = reserveMoreTrack(me, 0, me->d[speed][ACCELERATE][MAX_VAL]); // moving
      if (reserveStatus == RESERVE_FAIL) {
        reroute(me);
      }
    } else {
      //TrainDebug(me, "Set speed. %d %d", speed, me->trainNum);
      msg[0] = (char)speed;
      Putstr(me->com1, msg, 2);
      if (speed == 0) {
        int delayTime = stopTime + 500;
        Reply(me->stopDelayer, (char*)&delayTime, 4);
      }
    }
    if (speed > me->speed) {
      me->speedDir = ACCELERATE;
    } else if (speed < me->speed) {
      me->speedDir = DECELERATE;
    }
    me->speed = speed;
  } else {
    //TrainDebug(me, "Reverse... %d ", me->speed);
    DriverMsg delayMsg;
    delayMsg.type = SET_SPEED;
    delayMsg.timestamp = stopTime + 500;
    if (me->speedAfterReverse == -1) {
      delayMsg.data2 = (signed char)me->speed;
    } else {
      delayMsg.data2 = (signed char)me->speedAfterReverse;
    }
    //TrainDebug(me, "Using delayer: %d for %d", me->delayer, stopTime);

    Reply(me->delayer, (char*)&delayMsg, sizeof(DriverMsg));

    msg[0] = 0;
    msg[1] = (char)me->trainNum;

    Putstr(me->com1, msg, 2);
    me->speed = 0;
    me->speedDir = DECELERATE;
  }
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
      dPosition = (time - me->lastPosUpdateTime) * getVelocity(me) / 100000.0;
    }
    me->lastPosUpdateTime = time;
    me->distanceFromLastSensor += dPosition;
    me->distanceToNextSensor -= dPosition;
  }
}

static void sendUiReport(Driver* me) {
  me->uiMsg.velocity = getVelocity(me) / 100;
  me->uiMsg.lastSensorUnexpected     = me->lastSensorUnexpected;
  me->uiMsg.lastSensorBox            = me->lastSensorBox;
  me->uiMsg.lastSensorVal            = me->lastSensorVal;
  me->uiMsg.lastSensorActualTime     = me->lastSensorActualTime;
  me->uiMsg.lastSensorIsTerminal     = me->lastSensorIsTerminal;

  me->uiMsg.speed                    = me->speed;      // 0 - 14
  me->uiMsg.speedDir                 = me->speedDir;
  me->uiMsg.distanceFromLastSensor   = (int)me->distanceFromLastSensor;
  me->uiMsg.distanceToNextSensor     = (int)me->distanceToNextSensor;

  me->uiMsg.nextSensorBox            = me->nextSensorBox;
  me->uiMsg.nextSensorVal            = me->nextSensorVal;
  me->uiMsg.nextSensorIsTerminal     = me->nextSensorIsTerminal;

  me->uiMsg.lastSensorDistanceError  = me->lastSensorDistanceError;
  me->uiMsg.nextSwitchToBeSetNum     = me->route.nodes[me->nextSetSwitchNode].landmark.num2;
  me->uiMsg.nextSwitchToBeSetState   = me->route.nodes[me->nextSetSwitchNode].num;

  Send(me->ui, (char*)&(me->uiMsg), sizeof(TrainUiMsg), (char*)1, 0);
}

static void initDriver(Driver* me, int firstTime) {
  char uiName[] = UI_TASK_NAME;
  me->ui = WhoIs(uiName);
  me->CC = 0;
  me->speedAfterReverse = -1;
  me->rerouteCountdown = -1;
  me->nextSetSwitchNode = -1;
  me->reserveFailedLandmark.type = LANDMARK_BAD;

  char trackName[] = TRACK_NAME;
  me->trackManager = WhoIs(trackName);
  me->route.length = 0;
  me->stopCommited = 0; // haven't enabled speed zero yet.
  me->useLastSensorNow = 0;
  me->stopNow = 0;
  me->positionFinding = 0;
  me->currentlyLost = 0;
  me->testMode = 0;
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
  char com1Name[] = IOSERVERCOM1_NAME;
  me->com1 = WhoIs(com1Name);
  me->uiMsg.type = UPDATE_TRAIN;

  me->speed = 0;
  me->speedDir = ACCELERATE;
  me->distanceToNextSensor = 0;
  me->distanceFromLastSensor = 0;
  me->lastSensorActualTime = 0;
  me->lastSensorDistanceError = 0;

  if (firstTime) {
    me->delayer = Create(1, trainDelayer);
    me->stopDelayer = Create(1, trainStopDelayer);
    me->sensorWatcher = Create(3, trainSensor);
    me->navigateNagger = Create(2, trainNavigateNagger);
  }
  me->routeRemaining = -1;

  me->isAding = 0;

  initStoppingDistance((int*)me->d);
  initVelocity((int*)me->v);
  initAccelerationProfile((int*)me->a);
}

void driver() {
  Driver me;
  initDriver(&me, 1);

  unsigned int naggCount = 0;
  unsigned int updateStoppingDistanceCount = 0;

  for (;;) {
    int tid = -1;
    DriverMsg msg;
    msg.data2 = -1;
    msg.data3 = -1;
    msg.replyTid = -1;
    Receive(&tid, (char*)&msg, sizeof(DriverMsg));
    if (tid != me.delayer && tid != me.stopDelayer) {
      Reply(tid, (char*)1, 0);
    }
    const int replyTid = msg.replyTid;

    switch (msg.type) {
      case SET_SPEED: {
        //TrainDebug(&me, "Set speed from msg");
        trainSetSpeed(msg.data2,
                      getStoppingTime(&me),
                      (msg.data3 == DELAYER),
                      &me);
        if (msg.data3 != DELAYER) {
          //TrainDebug(&me, "Replied to %d", replyTid);
          Reply(replyTid, (char*)1, 0);
          sendUiReport(&me);
          break;
        } else if (me.route.length != 0) {
          // Delayer came back. Reverse command completed
          me.stopCommited = 0; // We're moving again.
          // We've completed everything up to the reverse node.
          me.routeRemaining = me.stopNode+1;
          me.previousStopNode = me.routeRemaining;
          me.distanceFromLastSensorAtPreviousStopNode = me.distanceFromLastSensor;
          // Calculate the next stop node.
          updateStopNode(&me);
          me.nextSetSwitchNode = -1;
          updateSetSwitch(&me);
          // if the reverse is last node, nothing to do
          // if it isn't.. it should speed up again.
        }
      }
      case DELAYER: {
        //TrainDebug(&me, "delayer come back.");
        break;
      }
      case STOP_DELAYER: {
        // To prevent the first receive from this delayer
        if (me.lastSensorActualTime > 0 && me.speed == 0 && !me.isAding) {
          TrainDebug(&me, "releasing reserveration");
          int reserveStatus = reserveMoreTrack(&me, 1, 0);
          if (reserveStatus == RESERVE_FAIL) {
            TrainDebug(&me, "WARNING: unable to reserve during init");
          }
        }
        break;
      }
      case SENSOR_TRIGGER: {

        // only handle sensor reports in primary + secondary prediction if not position finding
        int sensorReportValid = 0;
        TrackLandmark conditionLandmark;
        int condition;
        int isSensorReserved = QueryIsSensorReserved(&me, msg.data2, msg.data3);
        if (me.positionFinding) {
          sensorReportValid = 1;
          me.lastSensorUnexpected = 1;
          FinishPositionFinding(me.trainNum, me.trainController);
        } else if (isSensorReserved) {
          //TrainDebug(&me, "Predictions.");
          for (int i = 0; i < me.numPredictions; i ++) {
            TrackLandmark predictedSensor = me.predictions[i].sensor;
            //printLandmark(&me, &predictedSensor);
            if (predictedSensor.type == LANDMARK_SENSOR && predictedSensor.num1 == msg.data2 && predictedSensor.num2 == msg.data3) {
              sensorReportValid = 1;
              if (i != 0) {
                TrainDebug(&me, "Trigger Secondary");
                // secondary prediction, need to do something about them
                conditionLandmark = me.predictions[i].conditionLandmark;
                condition = me.predictions[i].condition;
                me.lastSensorUnexpected = 1;
                if (conditionLandmark.type == LANDMARK_SWITCH) {
                  TrackMsg setSwitch;
                  setSwitch.type = UPDATE_SWITCH_STATE;
                  TrainDebug(&me, "UPDATE SWITCH STATE");
                  setSwitch.landmark1 = conditionLandmark;
                  setSwitch.data = condition;

                  Send(me.trackManager, (char*)&setSwitch, sizeof(TrackMsg), (char *)1, 0);
                }

                // Stop and then try to reroute.
                reroute(&me);
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
          me.lastSensorIsTerminal = 0;
          me.lastSensorActualTime = msg.timestamp;
          dynamicCalibration(&me);
          me.lastSensorPredictedTime = me.nextSensorPredictedTime;

          TrackNextSensorMsg trackMsg;
          QueryNextSensor(&me, &trackMsg);
          // Reserve the track above train and future (covers case of init)

          for (int i = 0; i < trackMsg.numPred; i++) {
            me.predictions[i] = trackMsg.predictions[i];
          }
          me.numPredictions = trackMsg.numPred;

          int reserveStatus = reserveMoreTrack(&me, me.positionFinding, getStoppingDistance(&me));
          if (reserveStatus == RESERVE_FAIL) {
            if (!me.positionFinding) {
              reroute(&me);
            } else {
              TrainDebug(&me, "WARNING: unable to reserve during init");
            }
          }

          TrackSensorPrediction primaryPrediction = me.predictions[0];
          me.calibrationStart = msg.timestamp;
          me.calibrationDistance = primaryPrediction.dist;
          int dPos = 50 * getVelocity(&me) / 100000.0;
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
            getVelocity(&me);

          updatePosition(&me, msg.timestamp);
          sendUiReport(&me);
          if (me.positionFinding) {
            trainSetSpeed(0, getStoppingTime(&me), 0, &me); // Found position, stop.
            me.positionFinding = 0;
            me.currentlyLost = 0;
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
                //TrainDebug(&me, "Navi reversing.");
                const int speed = -1;
                trainSetSpeed(speed, getStoppingTime(&me), 0, &me);
              }
              else {
                //TrainDebug(&me, "Navi Nagger stopping.");
                const int speed = 0;  // Set speed zero.
                trainSetSpeed(speed, getStoppingTime(&me), 0, &me);
                me.route.length = 0; // Finished the route.
                me.testMode = 0;
              }
              me.stopCommited = 1;
              me.useLastSensorNow = 0;
              me.stopNow = 0;
              me.stopSensorHit = 0;
            } else {
              if ((++updateStoppingDistanceCount & 15) == 0) updateStopNode(&me);
            }
          }
        }

        if (me.nextSetSwitchNode != -1 && (++me.setSwitchNaggerCount & 3) == 0) {
          trySetSwitch_and_getNextSwitch(&me);
        }
        if (me.rerouteCountdown-- == 0) {
          if (me.testMode) {
            int reserveStatus = reserveMoreTrack(&me, 0, me.d[8][ACCELERATE][MAX_VAL]); // moving
            if (reserveStatus == RESERVE_FAIL) {
              reroute(&me);
            } else {
              me.nextSetSwitchNode = -1;
              updateSetSwitch(&me);
              trainSetSpeed(8, 0, 0, &me);
            }
          } else {
            // reroute
            if (me.route.length != 0) {
              setRoute(&me, &(me.routeMsg));
            }
          }
        }
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
        int reserveStatus = reserveMoreTrack(&me, 0, getStoppingDistance(&me)); // moving
        if (reserveStatus == RESERVE_FAIL) {
          reroute(&me);
        }
        break;
      }
      case BROADCAST_TEST_MODE: {
        me.testMode = 1;
        setRoute(&me, &msg);
        break;
      }
      case FIND_POSITION: {
        me.positionFinding = 1;
        trainSetSpeed(5, 0, 0, &me);
        break;
      }
      default: {
        TrainDebug(&me, "Not suppported train message type.");
      }
    }
  }
}

#include <DriverHelper.c>
