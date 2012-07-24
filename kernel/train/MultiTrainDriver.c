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
#include <MultiTrainDriver.h>
#include <DumbDriver.h>
#include <Lock.h>

static void printLandmark(Driver* me, TrackLandmark* l);
static void trainDelayer();
static void trainStopDelayer();
static void trainSensor();
static void trainNavigateNagger();
static void printRoute(Driver* me);
static void QueryNextSensor(Driver* me, TrackNextSensorMsg* trackMsg);
static int QueryIsSensorReserved(Driver* me, int box, int val);
static void setRoute(Driver* me, DriverMsg* msg);
static void updatePrediction(Driver* me);
static int reserveMoreTrack(Driver* me, int stopped, int stoppingDistance);
static void multiTrainDriverCourier();

static void reroute(Driver* me) {
  //TODO
}

static void sendUiReport(Driver* me) {

}

static int getStoppingDistance(Driver* me) {
  return 440; // TODO
}

static int interpolateStoppingDistance(Driver* me, int velocity) {
  return 440; // TODO
}

static int getVelocity(Driver* me) {
  return 56000; // TODO
}

static void trainSetSpeed(const int speed, const int stopTime, const int delayer, Driver* me) {
}

static void clearReservation(MultiTrainDriver* me) {
  if (!me->tailMode) {
    PrintDebug(me->driver.ui, "%d Cannot clear reservation in head mode", me->driver.trainNum);
    return;
  }
  ReleaseOldAndReserveNewTrackMsg qMsg;
  qMsg.type = RELEASE_ALL_RESERVATION;
  qMsg.trainNum = me->driver.trainNum;

  Send(me->driver.trackManager,
      (char*)&qMsg, sizeof(ReleaseOldAndReserveNewTrackMsg),
      (char*)1, 0);
}

static int makeReservation(MultiTrainDriver* me) {
  if (me->tailMode) {
    PrintDebug(me->driver.ui, "Cannot make reservation in tail mode");
    return 0;
  }
  int isStationary = me->stoppedCount == me->numTrainInGroup;
  TrackLandmark sensors[MAX_TRAIN_IN_GROUP * 10];
  int sensorIndex = 0;
  // bad merging code here
  for (int i = 0; i < me->numTrainInGroup; i++) {
    int numSensor = isStationary ? 1 : me->numSensorToReserve[i];
    for (int j = 0; j < numSensor; j++) {
      int isInQueue = 1;
      for (int k = 0; k < sensorIndex; k++) {
        if (sensors[k].type == me->sensorToReserve[i][j].type &&
            sensors[k].num2 == me->sensorToReserve[i][j].num1 &&
            sensors[k].num2 == me->sensorToReserve[i][j].num2) {
          isInQueue = 0;
          break;
        }
      }
      sensors[sensorIndex++] = me->sensorToReserve[i][j];
    }
  }

  ReleaseOldAndReserveNewTrackMsg qMsg;
  qMsg.type = RELEASE_OLD_N_RESERVE_NEW;
  qMsg.trainNum = me->driver.trainNum;
  qMsg.stoppingDistance = isStationary ? 1 : getStoppingDistance(&me->driver);
  qMsg.lastSensor = sensors[0];

  //TrainDebug(me, "Reserving track");
  qMsg.numPredSensor = sensorIndex - 1;
  for(int i = 1; i < sensorIndex; i++) {
    qMsg.predSensor[i-1] = sensors[i];
    //printLandmark(me, &qMsg.predSensor[i]);
  }

  // reserveFailedlandmark is not really being used right now
  int len = Send(
      me->driver.trackManager,
      (char*)&qMsg, sizeof(ReleaseOldAndReserveNewTrackMsg),
      (char*)&(me->driver.reserveFailedLandmark), sizeof(TrackLandmark));
  if (len > 0) {
    return RESERVE_FAIL;
  } else {
    return RESERVE_SUCESS;
  }
}


// copied from driver......
// TODO, clean up of things this doesn't need
static void initDriver(MultiTrainDriver* me) {

  char uiName[] = UI_TASK_NAME;
  me->driver.ui = WhoIs(uiName);
  me->driver.CC = 0;
  me->driver.speedAfterReverse = -1;
  me->driver.rerouteCountdown = -1;
  me->driver.nextSetSwitchNode = -1;
  me->driver.reserveFailedLandmark.type = LANDMARK_BAD;

  char trackName[] = TRACK_NAME;
  me->driver.trackManager = WhoIs(trackName);
  me->driver.route.length = 0;
  me->driver.stopCommited = 0; // haven't enabled speed zero yet.
  me->driver.useLastSensorNow = 0;
  me->driver.stopNow = 0;
  me->driver.currentlyLost = 0;
  me->driver.testMode = 0;
  me->driver.stopSensorHit = 0;
  me->driver.nextSensorIsTerminal = 0;
  me->driver.lastSensorIsTerminal = 0;
  me->driver.lastSensorVal = 0; // Note to ui to don't print sensor.
  me->driver.setSwitchNaggerCount = 0;
  me->isReversing = 0;

  char timename[] = TIMESERVER_NAME;
  me->driver.timeserver = WhoIs(timename);

  MultiTrainInitMsg init;
  Receive(&(me->driver.trainController), (char*)&init, sizeof(MultiTrainInitMsg));
  Reply(me->driver.trainController, (char*)1, 0);

  // Create dumb drivers
  me->tailMode = 0;
  me->numTrainInGroup = 1;
  me->trainId[0] = CreateDumbTrain(init.nth, (int)init.trainNum);
  me->driver.trainNum = init.trainNum;
  RegisterMulti(init.trainNum);

  me->driver.speed = 0;
  me->driver.speedDir = ACCELERATE;
  me->driver.distanceToNextSensor = 0;
  me->driver.distanceFromLastSensor = 0;
  me->driver.lastSensorActualTime = 0;
  me->driver.lastSensorDistanceError = 0;

  me->driver.sensorWatcher = Create(3, trainSensor);
  me->courier = Create(3, multiTrainDriverCourier);
  //me->driver.navigateNagger = Create(2, trainNavigateNagger);

  me->driver.routeRemaining = -1;
  me->stoppedCount = 0;
}

void dumbDriverInfoUpdater() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = INFO_UPDATE_NAGGER;

  for (;;) {
    Delay(5, timeserver);
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

static void multiTrainDriverCourier() {
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = MULTI_TRAIN_DRIVER_COURIER;
  for (;;) {
    MultiTrainDriverCourierMsg cMsg;
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)&cMsg, sizeof(MultiTrainDriverCourierMsg));
    Send(cMsg.destTid, (char*)&cMsg.msg, sizeof(MultiTrainDriverMsg), (char *)1, 0);
  }
}

static void groupSetSpeed(MultiTrainDriver* me, DriverMsg* msg) {
  for (int i = 0; i < me->numTrainInGroup; i++) {
    Send(me->trainId[i], (char *)msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

void multitrain_driver() {
  MultiTrainDriver me;

  initDriver(&me);
  int initCounter = 0; // TODO, hack, train need to be properly inited
  for (;;) {
    int tid = -1;
    MultiTrainDriverMsg actualMsg;
    DriverMsg* msg = (DriverMsg*)&actualMsg;
    Receive(&tid, (char *)msg, sizeof(MultiTrainDriverMsg));
    if (msg->type != REPORT_INFO && msg->type !=  QUERY_STOP_COUNT && msg->type != MULTI_TRAIN_DRIVER_COURIER && msg->type != SENSOR_TRIGGER) {
      Reply(tid, (char*)1, 0);
    }

    switch (msg->type) {
      case SET_SPEED: {
        PrintDebug(me.driver.ui, "Set speed");
        // Head train replied and calculate speed.
        if (!me.tailMode) {
          PrintDebug(me.driver.ui, "head mode");
          Reply(msg->replyTid, (char*)1, 0);
          if (msg->data2 == -1) {
            // make every train stop and wait for there stop response
            msg->data2 = 0;
            me.isReversing = 1;
            me.speedAfterReverse = me.info[0].trainSpeed;
            me.stoppedCount = 0; // required hack, the count will be accumulated back to numTrain soon
          }

          if (msg->data2 != -1 && msg->data2 != 0) {
            // everyone is moving again
            me.stoppedCount = 0;
            if (me.info[0].trainSpeed == 0) {
              makeReservation(&me);
            }
          }
        } else  {
          //Send(me->trainId[0], (char *)msg, sizeof(DriverMsg), (char*)1, 0);
          PrintDebug(me.driver.ui, "tail mode");
        }

        // TODO(rcao) may only want to send to head train and tail trains coorperate
        // NOTE:zhang MERGE_TAIL mode should be in sync with here.
        groupSetSpeed(&me, msg);
        break;
      }
      case SENSOR_TRIGGER: {
        if (me.tailMode && tid == me.driver.sensorWatcher) {
          Reply(tid, (char *)NULL, 0);
          break;
        }

        if (me.tailMode) {
          int isHandled = 0;
          Send(me.trainId[0],
              (char*)msg, sizeof(DriverMsg), (char *)&isHandled, sizeof(int));
          Reply(tid, (char *)&isHandled, sizeof(int));
        } else {
          int isSensorReserved = QueryIsSensorReserved(&me.driver, msg->data2, msg->data3);
          if (isSensorReserved) {
            // sensor is reserved
            for (int i = 0; i < me.numTrainInGroup; i ++) {
              int isHandled = 0;
              Send(me.trainId[i],
                  (char*)msg, sizeof(DriverMsg), (char *)&isHandled, sizeof(int));
              if (isHandled) {
                break;
              }
            }
          }
          Reply(tid, (char *)NULL, 0);
        }
        break;
      } // case
      case INFO_UPDATE_NAGGER: {
        if (me.tailMode) break;

        DriverMsg dMsg;
        dMsg.type = REPORT_INFO;
        for (int i = 0; i < me.numTrainInGroup; i++) {
          Send(me.trainId[i],
              (char *)&dMsg, sizeof(DriverMsg),
              (char*)&me.info[i], sizeof(DumbDriverInfo));
        }

        // check train's relative position difference
        for (int i = 1; i < me.numTrainInGroup; i++) {
          int distance = 0;
          TrackMsg tMsg;
          tMsg.type = QUERY_DISTANCE;
          tMsg.position1 = me.info[i].pos;
          tMsg.position2 = me.info[i-1].pos;
          Send(me.driver.trackManager, (char*)&tMsg, sizeof(TrackMsg), (char *)&distance, sizeof(int));

          // This is pretty arbitrary now and needs tuning
          if (distance > 2 * 180 + 100 && me.info[i].trainSpeed < me.info[i-1].trainSpeed + 1 && me.info[i].trainSpeed < 14) {
            PrintDebug(me.driver.ui, "Speeding up Distance: %d", distance);
            // too far, back train need to speed up
            dMsg.type = SET_SPEED;
            dMsg.data2 = me.info[i].trainSpeed + 1;
            dMsg.data3 = -1;
            Send(me.trainId[i], (char*)&dMsg, sizeof(DriverMsg), (char*)1, 0);
          } else if (distance < 2 * 180 && distance > 0 && me.info[i].trainSpeed > me.info[i-1].trainSpeed - 1 && me.info[i].trainSpeed > 0) {
            // too close, back train need to slow up
            PrintDebug(me.driver.ui, "Slowing down Distance %d", distance);
            // too far, back train need to speed up
            dMsg.type = SET_SPEED;
            dMsg.data2 = me.info[i].trainSpeed - 1;
            dMsg.data3 = -1;
            Send(me.trainId[i], (char*)&dMsg, sizeof(DriverMsg), (char*)1, 0);
          }
        }
        break;
      }
      case UPDATE_PREDICTION: {
        if (me.tailMode) {
          MultiTrainDriverCourierMsg cMsg;
          cMsg.destTid = me.headTid;
          cMsg.msg = actualMsg;
          cMsg.msg.data = MyTid();
          Reply(me.courier, (char *)&cMsg, sizeof(MultiTrainDriverCourierMsg));
          break;
        }

        for (int i = 0; i < MAX_TRAIN_IN_GROUP; i++) {
          if (actualMsg.data == me.trainId[i]) {
            for (int j = 0; j < actualMsg.numSensors; j++) {
              me.sensorToReserve[i][j] = actualMsg.sensors[j];
            }
            me.numSensorToReserve[i] = actualMsg.numSensors;
          }
        }
        makeReservation(&me);
        break;
      }
      case STOP_COMPLETED: {
        // notify actual train controller.
        if (me.tailMode) {
          MultiTrainDriverCourierMsg cMsg;
          cMsg.destTid = me.headTid;
          cMsg.msg = actualMsg;
          cMsg.msg.data = MyTid();
          Reply(me.courier, (char *)&cMsg, sizeof(MultiTrainDriverCourierMsg));
          break;
        }
        me.stoppedCount++;

        if (me.stoppedCount == me.numTrainInGroup) {
          makeReservation(&me);
          if (me.isReversing) {
            // flip the heads and tails of everythingggg
            for (int i = 0; i < me.numTrainInGroup / 2; i++) {
              int tempTrainId = me.trainId[i];
              DumbDriverInfo tempInfo = me.info[i];
              TrackLandmark tempSensorToReserve[10];
              for (int j = 0; j < 10; j++) {
                tempSensorToReserve[j] = me.sensorToReserve[i][j];
              }
              int tempNumSensorToReserve = me.numSensorToReserve[i];

              int dest = me.numTrainInGroup - i - 1;
              me.trainId[i] = me.trainId[dest];
              me.info[i] = me.info[dest];
              for (int j = 0; j < 10; j++) {
                me.sensorToReserve[i][j] = me.sensorToReserve[dest][j];
              }
              me.numSensorToReserve[i] = me.numSensorToReserve[dest];

              me.trainId[dest] = tempTrainId;
              me.info[dest] = tempInfo;
              for (int j = 0; j < 10; j++) {
                me.sensorToReserve[dest][j] = tempSensorToReserve[j];
              }
              me.numSensorToReserve[dest] = tempNumSensorToReserve;
            }

            DriverMsg dMsg;
            dMsg.type = REVERSE_SPEED;
            dMsg.data2 = me.speedAfterReverse;
            // inform everyone to reverse now
            for (int i = 0; i < me.numTrainInGroup; i++) {
              Send(me.trainId[i], (char *)&dMsg, sizeof(DriverMsg), (char *)NULL, 0);
            }
            me.isReversing = 0;
            if (me.speedAfterReverse != 0) {
              me.stoppedCount = 0;
            }
          }
        }
        break;
      }
      case NAVIGATE_NAGGER: {
        // TODO
        break;
      }
      case FIND_POSITION: {
        if (me.tailMode) {
          PrintDebug(me.driver.ui, "find position while merge????");
          break;
        }

        PrintDebug(me.driver.ui, "Train locking %d", me.driver.trainNum);
        // Only 1 train can lock at the same time.
        lock(me.driver.timeserver);
        // begin finding position in a slow speed
        DumbTrainSetSpeed(me.trainId[0], 5);
        Reply(msg->replyTid, (char*)1, 0);
        for (;;) {
          Receive(&tid, (char*)msg, sizeof(MultiTrainDriverMsg));
          Reply(tid, (char*)1, 0);
          if (msg->type == SENSOR_TRIGGER) {
            Send(me.trainId[0], (char*)msg,
                sizeof(DriverMsg), (char*)NULL, 0);
            DumbTrainSetSpeed(me.trainId[0], 0);
            break;
          }
          PrintDebug(me.driver.ui, "WARNN Drop %d", msg->type);
        }
        Create(3, dumbDriverInfoUpdater);
        unlock();
        break;
      }
      case MERGE_HEAD: {
        PrintDebug(me.driver.ui, "merge head");
        if (me.tailMode) {
          PrintDebug(me.driver.ui, "Cannot be a head when in tail mode??");
          break;
        }
        // Other train controller's id.
        me.trainId[me.numTrainInGroup] = msg->data2;
        me.numTrainInGroup++;
        Reply(msg->replyTid, (char*)1, 0);
        DriverMsg dMsg;
        dMsg.type = QUERY_STOP_COUNT;
        int tailStopCount = 0;
        Send(msg->data2, (char *)&dMsg, sizeof(DriverMsg), (char *)&tailStopCount, sizeof(int));
        me.stoppedCount += tailStopCount;

        dMsg.type = UPDATE_PARENT_ABOUT_PREDICTION;
        Send(msg->data2, (char *)&dMsg, sizeof(DriverMsg), (char *)NULL, 0);

        PrintDebug(me.driver.ui, "merge head done");
        break;
      }
      case MERGE_TAIL: {
        PrintDebug(me.driver.ui, "merge tail begin");
        if (me.tailMode) {
          PrintDebug(me.driver.ui, "Double merge tail??");
          break;
        }
        // Enters courier mode that passes dumb_train msg to 'real' controller
        me.tailMode = 1;
        me.headTid = msg->data2;
        clearReservation(&me);
        Reply(msg->replyTid, (char*)1, 0);
        PrintDebug(me.driver.ui, "merge tail done");
        break;
      }
      case SEPARATE_TAIL: {
        if (!me.tailMode) {
          PrintDebug(me.driver.ui, "Not in tail mode..??"); break;
        }
        me.tailMode = 0;
        me.headTid = 0;
        // TODO, behaviour is not clearly defined yet.
        // reserve my own track and prediction??
        break;
      }
      case REPORT_INFO: {
        if (!me.tailMode) {
          PrintDebug(me.driver.ui, "Report info Not in tail mode..??"); break;
        }

        // ASSUME ONLY 1 train when in tail mode.
        Send(me.trainId[0], (char*)msg, sizeof(DriverMsg),
            (char*)&me.info[0], sizeof(DumbDriverInfo));
        // Reply the head that made query.
        Reply(me.headTid, (char*)&me.info[0], sizeof(DumbDriverInfo));
        break;
      }
      case UPDATE_PARENT_ABOUT_PREDICTION: {
        if (!me.tailMode) {
          PrintDebug(me.driver.ui, "Update Parent Not in tail mode..??"); break;
        }
        // ASSUME ONLY 1 train when in tail mode.
        Send(me.trainId[0], (char*)msg, sizeof(DriverMsg),
            (char*)NULL, 0);
        break;
      }
      case QUERY_STOP_COUNT: {
        // asuumption: no tree strucutre
        Reply(tid, (char *)&me.stoppedCount, sizeof(int));
        break;
      }
      case MULTI_TRAIN_DRIVER_COURIER: {
        // nothing
        break;
      }
      case REVERSE_SPEED : {
        if (!me.tailMode) {
          PrintDebug(me.driver.ui, "Reverse Speed Not in tail mode..??"); break;
        }

        for (int i = 0; i < me.numTrainInGroup; i++) {
          Send(me.trainId[i], (char *)msg, sizeof(DriverMsg), (char*)1, 0);
        }
        break;
      }
      default: {
        PrintDebug(me.driver.ui, "Not Handled %d", msg->type);
      }
    } // switch
  } // for
}

#include <DriverHelper.c> // Don't really need everything in there....

int createMultitrainDriver(int nth, int trainNum) {
  MultiTrainInitMsg init;
  init.nth = nth;
  init.trainNum = trainNum;

  // Create train task
  int tid = Create(4, multitrain_driver);
  Send(tid, (char*)&init, sizeof(MultiTrainInitMsg), (char*)1, 0);
  return tid;
}


void RegisterMulti(int trainNum){
  char name[] = MULTI_TRAIN_NAME;
  name[3] = '0'+trainNum/10;
  name[4] = '0'+trainNum%10;
  RegisterAs(name);
}

int WhoIsMulti(int trainNum) {
  char name[] = MULTI_TRAIN_NAME;
  name[3] = '0'+trainNum/10;
  name[4] = '0'+trainNum%10;
  return WhoIs(name);
}
