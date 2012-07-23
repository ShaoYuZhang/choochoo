#include "Driver.h"
#include "DumbDriver.h"
#include "Driver.h"

#include <IoServer.h>
#include <NameServer.h>
#include <ts7200.h>
#include <util.h>
#include <TimeServer.h>
#include <syscall.h>
#include <CalibrationData.h>
#include <Poly.h>
#include <MultiTrainDriver.h>

#include "DriverHelperTask.c"
static int getVelocity(DumbDriver* me);

static void sendUiReport(DumbDriver* me) {
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

  Send(me->ui, (char*)&(me->uiMsg), sizeof(TrainUiMsg), (char*)1, 0);
}

static void updateParentAboutPrediction(DumbDriver* me) {
  MultiTrainDriverMsg msg;
  msg.type = UPDATE_PREDICTION;
  msg.numSensors = me->numPredictions + 1;
  msg.sensors[0].type = me->lastSensorIsTerminal ? LANDMARK_END : LANDMARK_SENSOR;
  msg.sensors[0].num1 = me->lastSensorBox;
  msg.sensors[0].num2 = me->lastSensorVal;
  for (int i = 0; i < me->numPredictions; i++) {
    msg.sensors[i+1] = me->predictions[i].sensor;
  }
  Reply(me->courier, (char *)&msg, sizeof(MultiTrainDriverMsg));
}

static void QueryNextSensor(DumbDriver* me, TrackNextSensorMsg* trackMsg) {
  TrackMsg qMsg;
  qMsg.type = QUERY_NEXT_SENSOR_FROM_SENSOR;
  qMsg.landmark1.type = LANDMARK_SENSOR;
  qMsg.landmark1.num1 = me->lastSensorBox;
  qMsg.landmark1.num2 = me->lastSensorVal;
  Send(me->trackManager, (char*)&qMsg, sizeof(TrackMsg),
      (char*)trackMsg, sizeof(TrackNextSensorMsg));
}

static void toPosition(DumbDriver* me, Position* pos) {
  pos->landmark1.type = me->lastSensorIsTerminal ? LANDMARK_END : LANDMARK_SENSOR;
  pos->landmark1.num1 = me->lastSensorBox;
  pos->landmark1.num2 = me->lastSensorVal;
  pos->landmark2.type = me->nextSensorIsTerminal ? LANDMARK_END : LANDMARK_SENSOR;
  pos->landmark2.num1 = me->nextSensorBox;
  pos->landmark2.num2 = me->nextSensorVal;
  pos->offset = (int)me->distanceFromLastSensor;
}

static void dumbDriverCourier() {
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = DRIVER_COURIER_INIT;
  int controllerId = 0;
  Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)&controllerId, sizeof(int));
  msg.type = DRIVER_COURIER;
  for (;;) {
    MultiTrainDriverMsg mMsg;
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)&mMsg, sizeof(mMsg));
    mMsg.data = parent;
    Send(controllerId, (char*)&mMsg, sizeof(MultiTrainDriverMsg), (char *)1, 0);
  }
}


static void initDriver(DumbDriver* me) {
  char uiName[] = UI_TASK_NAME;
  me->ui = WhoIs(uiName);
  me->speedAfterReverse = -1;
  char trackName[] = TRACK_NAME;
  me->trackManager = WhoIs(trackName);

  me->nextSensorIsTerminal = 0;
  me->lastSensorIsTerminal = 0;
  me->lastSensorVal = 0; // NOte to ui to don't print sensor.

  char timename[] = TIMESERVER_NAME;
  me->timeserver = WhoIs(timename);

  DriverInitMsg init;
  Receive(&(me->multiTrainController), (char*)&init, sizeof(DriverInitMsg));
  Reply(me->multiTrainController, (char*)1, 0);
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
  me->navigateNagger = Create(2, trainNavigateNagger);
  me->courier = Create(2, dumbDriverCourier);

  me->isAding = 0;
  initStoppingDistance((int*)me->d);
  initVelocity((int*)me->v);
  initAccelerationProfile((int*)me->a);
}

static void updatePosition(DumbDriver* me, int time){
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

static int getStoppingDistance(DumbDriver* me) {
  return me->d[(int)me->speed][(int)me->speedDir][MAX_VAL];
}

// mm/s
static int getVelocity(DumbDriver* me){
  if (me->isAding) {
    int now = Time(me->timeserver) * 10;
    return eval_velo(&(me->adPoly), now);
  } else {
    return me->v[(int)me->speed][(int)me->speedDir];
  }
}

static int interpolateStoppingDistance(DumbDriver* me, int velocity) {
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

static int getStoppingTime(DumbDriver* me) {
  return 2* interpolateStoppingDistance(me, getVelocity(me)) * 100000 /
    getVelocity(me);
}

static void dynamicCalibration(DumbDriver* me) {
  if (me->isAding) return; // TODO doesn't cover all cases
  if (me->lastSensorUnexpected) return;
  if (me->speed == 0) return; // Cannot calibrate speed zero

  int dTime = me->lastSensorPredictedTime - me->lastSensorActualTime;
  if (dTime > 400) return;
  if (dTime < -400) return;
  int velocity =
    me->calibrationDistance * 100 * 1000 /
    (me->lastSensorActualTime - me->calibrationStart);

  int originalVelocity = me->v[(int)me->speed][(int)me->speedDir];
  me->v[(int)me->speed][(int)me->speedDir]
      = (originalVelocity * 85 + velocity * 15) / 100;
}

static void trainSetSpeed(
    const int speed, const int stopTime, const int delayer, DumbDriver* me) {
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
    } else {
      //TrainDebug(me, "Set speed. %d %d", speed, me->trainNum);
      msg[0] = (char)speed;
      Putstr(me->com1, msg, 2);
      //if (speed == 0) {
      //  int delayTime = stopTime + 500;
      //  Reply(me->stopDelayer, (char*)&delayTime, 4);
      //}
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

void dumb_driver() {
  DumbDriver me;
  initDriver(&me);

  unsigned int naggCount = 0;

  for (;;) {
    int tid = -1;
    DriverMsg msg;
    msg.data2 = -1;
    msg.data3 = -1;
    msg.replyTid = -1;
    Receive(&tid, (char*)&msg, sizeof(DriverMsg));

    switch (msg.type) {
      case SET_SPEED: {
        //TrainDebug(&me, "Set speed from msg");
        trainSetSpeed(msg.data2,
                      getStoppingTime(&me),
                      (msg.data3 == DELAYER),
                      &me);
        Reply(tid, (char*)1, 0);
        break;
      }
      case DELAYER: {
        //TrainDebug(&me, "delayer come back.");
        break;
      }
      case SENSOR_TRIGGER: {
        TrainDebug(&me, "sensor reports");
        int sensorReportValid = 0;
        TrackLandmark conditionLandmark;
        int condition;
        if (me.lastSensorActualTime > 0) {
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
                //reroute(&me); // TODO, what TODO
              } else {
                me.lastSensorUnexpected = 0;
              }
            }
          }
        } else {
          sensorReportValid = 1;
        }

        if (sensorReportValid) {
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
          updateParentAboutPrediction(&me);

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
          int reportHandled = 1;
          Reply(tid, (char*)&reportHandled, sizeof(int));
        } else {
          int reportHandled = 0;
          Reply(tid, (char*)&reportHandled, sizeof(int));
        }
        break;
      }
      case NAVIGATE_NAGGER: {
        updatePosition(&me, msg.timestamp);
        if ((++naggCount & 15) == 0) sendUiReport(&me);
        Reply(tid, (char*)1, 0);

        break;
      }
      case FIND_POSITION: {
        trainSetSpeed(5, 0, 0, &me);
        Reply(tid, (char*)1, 0);
        break;
      }
      case REPORT_INFO: {
        DumbDriverInfo info;
        info.trainSpeed = me.speed;
        info.velocity = getVelocity(&me);
        info.maxStoppingDistance = getStoppingDistance(&me);
        info.currentStoppingDistance = interpolateStoppingDistance(&me, getVelocity(&me));
        toPosition(&me, &info.pos);
        Reply(tid, (char*)&info, sizeof(DumbDriverInfo));
        break;
      }
      case DRIVER_COURIER_INIT: {
        Reply(tid, (char*)&me.multiTrainController, sizeof(int));
        break;
      }
      case DRIVER_COURIER: {
        // nothing
        break;
      }
      default: {
        TrainDebug(&me, "Not suppported train message type.");
      }
    }
  }
}


int CreateDumbTrain(int nth, int trainNum, int com1) {
  DriverInitMsg dumbInit;
  dumbInit.nth = nth;
  dumbInit.trainNum = trainNum;
  dumbInit.com1 = com1;

  // Create train task
  int tid = Create(4, dumb_driver);
  Send(tid, (char*)&dumbInit, sizeof(DriverInitMsg), (char*)1, 0);
  return tid;
}