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

static int makeReservation(MultiTrainDriver* me) {
  TrackLandmark sensors[MAX_TRAIN_IN_GROUP * 10];
  int sensorIndex = 0;
  // bad merging code here
  for (int i = 0; i < me->numTrainInGroup; i++) {
    for (int j = 0; j < me->numSensorToReserve[i]; j++) {
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
  qMsg.stoppingDistance = getStoppingDistance(&me->driver);
  qMsg.lastSensor = sensors[0];

  //TrainDebug(me, "Reserving track");
  qMsg.numPredSensor = sensorIndex - 1;
  for(int i = 1; i < sensorIndex; i++) {
    qMsg.predSensor[i-1] = sensors[i];
    //printLandmark(me, &qMsg.predSensor[i]);
  }

  // reserveFailedlandmark is not really being used right now
  int len = Send(
      me->driver.trackManager, (char*)&qMsg, sizeof(ReleaseOldAndReserveNewTrackMsg),
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
  me->driver.positionFinding = 0;
  me->driver.currentlyLost = 0;
  me->driver.testMode = 0;
  me->driver.stopSensorHit = 0;
  me->driver.nextSensorIsTerminal = 0;
  me->driver.lastSensorIsTerminal = 0;
  me->driver.lastSensorVal = 0; // Note to ui to don't print sensor.
  me->driver.setSwitchNaggerCount = 0;

  char timename[] = TIMESERVER_NAME;
  me->driver.timeserver = WhoIs(timename);

  MultiTrainInitMsg init;
  Receive(&(me->driver.trainController), (char*)&init, sizeof(MultiTrainInitMsg));
  Reply(me->driver.trainController, (char*)1, 0);

  // Create dumb drivers
  me->numTrainInGroup = init.numTrain;
  for (int i = 0; i < me->numTrainInGroup; i++) {
    me->trainId[i] = CreateDumbTrain(init.nth+i, (int)init.trainNum[i]);
  }

  me->driver.trainNum = init.trainNum[0];

  me->driver.speed = 0;
  me->driver.speedDir = ACCELERATE;
  me->driver.distanceToNextSensor = 0;
  me->driver.distanceFromLastSensor = 0;
  me->driver.lastSensorActualTime = 0;
  me->driver.lastSensorDistanceError = 0;

  me->driver.sensorWatcher = Create(3, trainSensor);
  //me->driver.navigateNagger = Create(2, trainNavigateNagger);

  me->driver.routeRemaining = -1;
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

void multitrain_driver() {
  MultiTrainDriver me;

  initDriver(&me);
  int initCounter = 0; // TODO, hack, train need to be properly inited
  for (;;) {
    int tid = -1;
    MultiTrainDriverMsg actualMsg;
    DriverMsg *msg = (DriverMsg *)&actualMsg;
    Receive(&tid, (char *)msg, sizeof(MultiTrainDriverMsg));
    if (tid != me.driver.delayer && tid != me.driver.stopDelayer) {
      Reply(tid, (char*)1, 0);
    }

    const int replyTid = msg->replyTid;
    switch( msg->type) {
      case SET_SPEED: {
        Reply(replyTid, (char*)1, 0);
        // TODO(rcao) may only want to send to head train and tail trains coorperate
        for (int i = 0; i < me.numTrainInGroup; i++) {
          Send(me.trainId[i], (char *)msg, sizeof(DriverMsg), (char*)1, 0);
        }
        break;
      }
      case SENSOR_TRIGGER: {
        if (initCounter < 2) {
          Send(me.trainId[initCounter], (char*)msg, sizeof(DriverMsg), (char *)NULL, 0);
          initCounter++;
        } else {
          int isSensorReserved = QueryIsSensorReserved(&me.driver, msg->data2, msg->data3);
          if (isSensorReserved) {
            // sensor is reserved
            for (int i = 0; i < me.numTrainInGroup; i ++) {
              int isHandled = 0;
              Send(me.trainId[i], (char*)msg, sizeof(DriverMsg), (char *)&isHandled, sizeof(int));
              if (isHandled) {
                break;
              }
            }
          }
        }
        if (initCounter == 2) {
          Create(3, dumbDriverInfoUpdater);
          initCounter++; // hacks
        }
        break;
      } // case
      case INFO_UPDATE_NAGGER: {
        DriverMsg dMsg;
        dMsg.type = REPORT_INFO;
        for (int i = 0; i < me.numTrainInGroup; i++) {
          Send(me.trainId[i], (char *)&dMsg, sizeof(DriverMsg), (char*)&me.info[i], sizeof(DumbDriverInfo));
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
        for (int i = 0; i < MAX_TRAIN_IN_GROUP; i++) {
          if (actualMsg.data == me.trainId[i]) {
            for (int j = 0; j < actualMsg.numSensors; j++) {
              me.sensorToReserve[i][j] = actualMsg.sensors[j];
            }
            me.numSensorToReserve[i] = actualMsg.numSensors;
          }
        }
        if (initCounter > 2) {
          makeReservation(&me);
        }
        break;
      }
      case NAVIGATE_NAGGER: {
        // TODO
        break;
      }
      default: {
        //PrintDebug(me.driver.ui, "Not Handled");
      }
    } // switch
  } // for
}

#include <DriverHelper.c> // Don't really need everything in there....
