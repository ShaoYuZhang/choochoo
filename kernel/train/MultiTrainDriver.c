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
  for (int i = 0; i < me->numTrainInGroup; i++) {
    DriverInitMsg dumbInit;
    dumbInit.nth = init.nth + i;
    dumbInit.trainNum = (int)init.trainNum[i];
    dumbInit.com1 = init.com1;

    // Create train task
    me->trainId[i] = Create(4, dumb_driver);
    Send(me->trainId[i],
        (char*)&dumbInit, sizeof(DriverInitMsg), (char*)1, 0);
  }

  me->driver.trainNum = init.trainNum[0];
  me->driver.com1 = init.com1;

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

void multitrain_driver() {
  MultiTrainDriver me;

  initDriver(&me);
  for (;;) {
    int tid = -1;
    MultiTrainDriverMsg actualMsg;
    DriverMsg *msg = (DriverMsg *)&actualMsg;
    Receive(&tid, (char *)msg, MAX(sizeof(DriverMsg), sizeof(MultiTrainDriverMsg)));
    if (tid != me.driver.delayer && tid != me.driver.stopDelayer) {
      Reply(tid, (char*)1, 0);
    }
    for (int i = 0; i < NUM_PREVIOUS_SENSOR; i++) {
      me.previousSensorCount[i] = 0;
    }

    const int replyTid = msg->replyTid;
    switch( msg->type) {
      case SET_SPEED: {
        // TODO(rcao) may only want to send to head train and tail trains coorperate
        for (int i = 0; i < me.numTrainInGroup; i++) {
          Send(me.trainId[i], (char *)msg, sizeof(DriverMsg), (char*)1, 0);
        }
        Reply(replyTid, (char*)1, 0);
        break;
      }
      case SENSOR_TRIGGER: {
        int sensorReportValid = 0;
        int sensorReportForFirstTrain = 0;
        int previousSensorIndex = -1;
        TrackLandmark conditionLandmark;
        int condition;
        int isSensorReserved = QueryIsSensorReserved(&me.driver, msg->data2, msg->data3);
        if (isSensorReserved) {
          PrintDebug(me.driver.ui, "Predictions.");
          for (int i = 0; i < me.driver.numPredictions; i ++) {
            TrackLandmark predictedSensor = me.driver.predictions[i].sensor;
            //printLandmark(&me, &predictedSensor);
            if (predictedSensor.type == LANDMARK_SENSOR && predictedSensor.num1 == msg->data2 && predictedSensor.num2 == msg->data3) {
              sensorReportValid = 1;
              if (i != 0) {
                PrintDebug(me.driver.ui, "Trigger Secondary");
                // secondary prediction, need to do something about them
                conditionLandmark = me.driver.predictions[i].conditionLandmark;
                condition = me.driver.predictions[i].condition;
                me.driver.lastSensorUnexpected = 1;
                if (conditionLandmark.type == LANDMARK_SWITCH) {
                  TrackMsg setSwitch;
                  setSwitch.type = UPDATE_SWITCH_STATE; PrintDebug(me.driver.ui, "UPDATE SWITCH STATE");
                  setSwitch.landmark1 = conditionLandmark;
                  setSwitch.data = condition;

                  Send(me.driver.trackManager, (char*)&setSwitch, sizeof(TrackMsg), (char *)1, 0);
                }

                // Stop and then try to reroute.
                //reroute(&me); TODO
              } else {
                me.driver.lastSensorUnexpected = 0;
              }
              sensorReportForFirstTrain = 1;
            }
          }
          for (int i = 0 ;i < 2; i++) {
            if (me.previousSensor[i].type == LANDMARK_SENSOR && me.previousSensor[i].num1 == msg->data2 && me.previousSensor[i].num2 == msg->data3) {
              sensorReportValid = 1;
              previousSensorIndex = i;
              break;
            }
          }
        }
        if (sensorReportValid) {
          if (sensorReportForFirstTrain) {
            //updateRoute(&me.driver, msg->data2, msg->data3); // TODO
            me.driver.lastSensorBox = msg->data2; // Box
            me.driver.lastSensorVal = msg->data3; // Val
            me.driver.lastSensorIsTerminal = 0;
            me.driver.lastSensorActualTime = msg->timestamp;
            me.driver.lastSensorPredictedTime = me.driver.nextSensorPredictedTime;

            TrackNextSensorMsg trackMsg;
            QueryNextSensor(&me.driver, &trackMsg);
            // Reserve the track above train and future (covers case of init)

            for (int i = 0; i < trackMsg.numPred; i++) {
              me.driver.predictions[i] = trackMsg.predictions[i];
            }
            me.driver.numPredictions = trackMsg.numPred;

            //int reserveStatus = reserveMoreTrack(&me.driver, 0, getStoppingDistance(&me.driver));
            int reserveStatus = reserveMoreTrack(&me.driver, 0, getStoppingDistance(&me.driver)); //TODO
            if (reserveStatus == RESERVE_FAIL) {
              //reroute(&me); // TODO
            }

            TrackSensorPrediction primaryPrediction = me.driver.predictions[0];
            //int dPos = 50 * getVelocity(&me) / 100000.0;
            //me.driver.lastSensorDistanceError =  -(int)me.driver.distanceToNextSensor - dPos; // TODO
            //me.driver.distanceFromLastSensor = dPos;
            //me.driver.distanceToNextSensor = primaryPrediction.dist - dPos;
            me.driver.lastPosUpdateTime = msg->timestamp;
            if (primaryPrediction.sensor.type != LANDMARK_SENSOR &&
                primaryPrediction.sensor.type != LANDMARK_END) {
              PrintDebug(me.driver.ui, "QUERY_NEXT_SENSOR_FROM_SENSOR ..bad");
            }
            me.driver.nextSensorIsTerminal = (primaryPrediction.sensor.type == LANDMARK_END);
            me.driver.nextSensorBox = primaryPrediction.sensor.num1;
            me.driver.nextSensorVal = primaryPrediction.sensor.num2;
            //me.driver.nextSensorPredictedTime =
            //  msg->timestamp + me.driver.distanceToNextSensor*100000 /
            //  getVelocity(&me.driver);

            TrackLandmark sensor = {LANDMARK_SENSOR, msg->data2, msg->data3};
            for (int i = 0; i < me.numTrainInGroup; i++) {
              if (me.previousSensorCount[i] == 0) {
                me.previousSensor[i] = sensor;
                me.previousSensorCount[i] = me.numTrainInGroup - 1;
              }
            }

            Send(me.trainId[0], (char *)msg, sizeof(DriverMsg), (char*)1, 0);
          } else {
            Send(me.trainId[me.numTrainInGroup - me.previousSensorCount[previousSensorIndex]], (char *)msg, sizeof(DriverMsg), (char*)1, 0);
            me.previousSensorCount[previousSensorIndex]--;
            // sensor is done, shift things foward
            if (me.previousSensorCount[previousSensorIndex] == 0) {
              for (int i = previousSensorIndex + 1; i < me.numTrainInGroup; i++) {
                me.previousSensor[i - 1] = me.previousSensor[i];
                me.previousSensorCount[i - 1]  = me.previousSensorCount[i];
              }
              me.previousSensorCount[NUM_PREVIOUS_SENSOR - 1] = 0;
            }
          }
        }
        break;
      } // case
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

#include <DriverHelper.c>

