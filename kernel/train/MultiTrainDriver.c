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

static void getRoute(MultiTrainDriver* me, Position* from, DriverMsg* msg);
static void printLandmark(MultiTrainDriver* me, TrackLandmark* l);
static void trainSensor();
static void trainNavigateNagger();
static void printRoute(MultiTrainDriver* me);
static void QueryNextSensor(MultiTrainDriver* me, TrackNextSensorMsg* trackMsg);
static int QueryIsSensorReserved(MultiTrainDriver* me, int box, int val);
static void setRoute(MultiTrainDriver* me, Position* from, DriverMsg* msg);
static void multiTrainDriverCourier();
static void updateStopNode(MultiTrainDriver* me);
static void trySetSwitch_and_getNextSwitch(MultiTrainDriver* me);
static void updateSetSwitch(MultiTrainDriver* me);
static void updatePrediction(MultiTrainDriver* me);
static int groupSetSpeed(MultiTrainDriver* me, int speed);
static int shouldStopNow(MultiTrainDriver* me);

static void reroute(MultiTrainDriver* me) {
  groupSetSpeed(me, 0);
  me->rerouteCountdown = GET_TIMER4() % 2 == 0 ? 45 : 55; // wait ~1 seconds then reroute.
}

static int makeReservation(MultiTrainDriver* me, int stoppingDistance) {
  if (me->tailMode) {
    PrintDebug(me->ui, "Cannot make reservation in tail mode");
    return 0;
  }
  int isStationary = me->stoppedCount == me->numTrainInGroup;
  TrackLandmark sensors[MAX_TRAIN_IN_GROUP * 10];
  int sensorIndex = 0;
  // bad merging code here
  for (int i = 0; i < me->numTrainInGroup; i++) {
    int numSensor = isStationary ? 1 : me->numSensorToReserve[i];
    for (int j = 0; j < numSensor; j++) {
      int isInQueue = 0;
      for (int k = 0; k < sensorIndex; k++) {
        if (sensors[k].type == me->sensorToReserve[i][j].type &&
            sensors[k].num1 == me->sensorToReserve[i][j].num1 &&
            sensors[k].num2 == me->sensorToReserve[i][j].num2) {
          isInQueue = 1;
          break;
        }
      }
      if (!isInQueue) {
        sensors[sensorIndex++] = me->sensorToReserve[i][j];
      }
    }
  }
  PrintDebug(me->ui, "WTFFF %d", sensorIndex);

  ReleaseOldAndReserveNewTrackMsg qMsg;
  qMsg.type = RELEASE_OLD_N_RESERVE_NEW;
  qMsg.trainNum = me->trainNum;
  qMsg.stoppingDistance = isStationary ? 1 : stoppingDistance;
  qMsg.lastSensor = sensors[0];

  //TrainDebug(me, "Reserving track");
  qMsg.numPredSensor = sensorIndex - 1;
  for(int i = 1; i < sensorIndex; i++) {
    qMsg.predSensor[i-1] = sensors[i];
    //printLandmark(me, &qMsg.predSensor[i-1]);
  }

  int previousLandmarkState = me->reserveFailedLandmark.type;
  // reserveFailedlandmark is not really being used right now
  int len = Send(
      me->trackManager,
      (char*)&qMsg, sizeof(ReleaseOldAndReserveNewTrackMsg),
      (char*)&(me->reserveFailedLandmark), sizeof(TrackLandmark));
  if (len > 0) {
    //TrainDebug(me, "Failed cuz couldn't get landmark");
    printLandmark(me, &me->reserveFailedLandmark);
    return RESERVE_FAIL;
  } else if (!isStationary &&
      previousLandmarkState != LANDMARK_BAD &&
      me->reserveFailedLandmark.type != LANDMARK_BAD){
    //TrainDebug(me, "Got landmark bad.");
    me->reserveFailedLandmark.type = LANDMARK_BAD;
  }
  return RESERVE_SUCESS;
}


static void initDriver(MultiTrainDriver* me) {

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
  me->testMode = 0;
  me->stopSensorHit = 0;
  me->setSwitchNaggerCount = 0;
  me->isReversing = 0;

  char timename[] = TIMESERVER_NAME;
  me->timeserver = WhoIs(timename);

  MultiTrainInitMsg init;
  Receive(&(me->trainController), (char*)&init, sizeof(MultiTrainInitMsg));
  Reply(me->trainController, (char*)1, 0);

  // Create dumb drivers
  me->infoUpdater = -1;
  me->tailMode = 0;
  me->numTrainInGroup = 1;
  me->trainId[0] = CreateDumbTrain(init.nth, (int)init.trainNum);
  me->trainNum = init.trainNum;
  RegisterMulti(init.trainNum);

  me->speed = 0;
  me->speedDir = ACCELERATE;

  me->sensorWatcher = Create(3, trainSensor);
  me->courier = Create(3, multiTrainDriverCourier);

  me->routeRemaining = -1;
  me->stoppedCount = 0;
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

static int groupSetSpeed(MultiTrainDriver* me, int speed) {
  int shouldSetSpeed = 1;
  if (!me->tailMode) {
    if (speed == -1) {
      // make every train stop and wait for there stop response
      speed = 0;
      me->isReversing = 1;
      me->speedAfterReverse = me->info[0].trainSpeed;
      me->stoppedCount = 0; // required hack, the count will be accumulated back to numTrain soon
    }

    if (speed != 0) {
      // everyone is moving again
      me->stoppedCount = 0;
      if (me->info[0].trainSpeed == 0) {
        int stoppingDistance;
        DriverMsg qMsg;
        qMsg.type = QUERY_STOPPING_DISTANCE;
        qMsg.data2 = speed;
        Send(me->trainId[0], (char *)&qMsg, sizeof(DriverMsg), (char*)&stoppingDistance, sizeof(int));
        PrintDebug(me->ui, "Stopping Dist: %d", stoppingDistance);

        shouldSetSpeed = makeReservation(me, stoppingDistance) == RESERVE_SUCESS;
        PrintDebug(me->ui, "Should set speed: %d", shouldSetSpeed);
      }
    }
  }

  DriverMsg msg;
  msg.type = SET_SPEED;
  msg.data2 = speed;
  if (shouldSetSpeed) {
    for (int i = 0; i < me->numTrainInGroup; i++) {
      Send(me->trainId[i], (char *)&msg, sizeof(DriverMsg), (char*)1, 0);
    }
  }
  return shouldSetSpeed;
}

static void updateInfo(MultiTrainDriver* me) {
  DriverMsg dMsg;
  dMsg.type = REPORT_INFO;
  for (int i = 0; i < me->numTrainInGroup; i++) {
    Send(me->trainId[i],
        (char *)&dMsg, sizeof(DriverMsg),
        (char*)&me->info[i], sizeof(DumbDriverInfo));
  }

  // check train's relative position difference
  for (int i = 1; i < me->numTrainInGroup; i++) {
    int distance = 0;
    QueryDistance(me->trackManager,
        &me->info[i].pos, &me->info[i-1].pos, &distance);
    // Get more accurate distance based on knowledge of train direciton.
    distance -= me->info[i].lenBackOfPickup;
    distance -= me->info[i-1].lenFrontOfPickup;
    distance -= PICKUP_LEN;

    // This is pretty arbitrary now and needs tuning
    if (distance > 150 && me->info[i].trainSpeed < me->info[i-1].trainSpeed + 1 && me->info[i].trainSpeed < 14) {
      PrintDebug(me->ui, "Speeding up Distance: %d", distance);
      // too far, back train need to speed up
      dMsg.type = SET_SPEED;
      dMsg.data2 = me->info[i].trainSpeed + 1;
      dMsg.data3 = -1;
      Send(me->trainId[i], (char*)&dMsg, sizeof(DriverMsg), (char*)1, 0);
    } else if (distance < 100 && distance > 0 && me->info[i].trainSpeed > me->info[i-1].trainSpeed - 1 && me->info[i].trainSpeed > 0) {
      // too close, back train need to slow up
      PrintDebug(me->ui, "Slowing down Distance %d", distance);
      // too far, back train need to speed up
      dMsg.type = SET_SPEED;
      dMsg.data2 = me->info[i].trainSpeed - 1;
      dMsg.data3 = -1;
      Send(me->trainId[i], (char*)&dMsg, sizeof(DriverMsg), (char*)1, 0);
    }
  }
}

void multitrain_driver() {
  MultiTrainDriver me;

  initDriver(&me);
  unsigned int naggCount = 0;
  unsigned int updateStoppingDistanceCount = 0;

  for (;;) {
    int tid = -1;
    MultiTrainDriverMsg actualMsg;
    DriverMsg* msg = (DriverMsg*)&actualMsg;
    msg->data3 = 0;
    Receive(&tid, (char *)msg, sizeof(MultiTrainDriverMsg));
    if (msg->type != REPORT_INFO && msg->type !=  QUERY_STOP_COUNT && msg->type != MULTI_TRAIN_DRIVER_COURIER && msg->type != SENSOR_TRIGGER) {
      Reply(tid, (char*)1, 0);
    }

    switch (msg->type) {
      case SET_SPEED: {
        PrintDebug(me.ui, "Set speed");
        if (!me.tailMode) {
          Reply(msg->replyTid, (char*)1, 0);
        }

        groupSetSpeed(&me, msg->data2);
        break;
      }
      case SENSOR_TRIGGER: {
        if (me.tailMode && tid == me.sensorWatcher) {
          Reply(tid, (char *)NULL, 0);
          break;
        }

        if (me.tailMode) {
          int isHandled = 0;
          Send(me.trainId[0],
              (char*)msg, sizeof(DriverMsg), (char *)&isHandled, sizeof(int));
          Reply(tid, (char *)&isHandled, sizeof(int));
        } else {
          int isSensorReserved = QueryIsSensorReserved(&me, msg->data2, msg->data3);
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
      case NAVIGATE_NAGGER: {
        if (me.tailMode) break;

        updateInfo(&me);

        if (me.routeRemaining != -1) {
          if (!me.stopCommited) {
            if (shouldStopNow(&me)) {
              if (me.route.nodes[me.stopNode].num == REVERSE) {
                //TrainDebug(&me, "Navi reversing.");
                groupSetSpeed(&me, -1); // reverse
              }
              else {
                //TrainDebug(&me, "Navi Nagger stopping.");
                groupSetSpeed(&me, 0); // stopping
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
            int reserveStatus = makeReservation(&me, 440);
            if (reserveStatus == RESERVE_FAIL) {
              reroute(&me);
            } else {
              me.nextSetSwitchNode = -1;
              updateSetSwitch(&me);
              groupSetSpeed(&me, 8);
            }
          } else {
            // reroute
            if (me.route.length != 0) {
              setRoute(&me, &(me.info[0].pos), &(me.routeMsg));
            }
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
        makeReservation(&me, me.info[0].maxStoppingDistance);
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
          makeReservation(&me, 1);
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
            updateInfo(&me);
            if (me.speedAfterReverse != 0) {
              me.stoppedCount = 0;
            }
            if (me.route.length != 0) {
              // Delayer came back. Reverse command completed
              me.stopCommited = 0; // We're moving again.
              // We've completed everything up to the reverse node.
              me.routeRemaining = me.stopNode+1;
              me.previousStopNode = me.routeRemaining;
              me.distanceFromLastSensorAtPreviousStopNode = me.info[0].pos.offset;
              // Calculate the next stop node.
              updateStopNode(&me);
              me.nextSetSwitchNode = -1;
              updateSetSwitch(&me);
            }
          }
        }
        break;
      }
      case SET_ROUTE: {
        Reply(msg->replyTid, (char*)1, 0);
        me.routeMsg = *msg;
        setRoute(&me, &(me.info[0].pos), msg);
        break;
      }
      case GET_POSITION: {
        if (me.tailMode) {
          PrintDebug(me.ui, "Get position from a tail train ??");
          break;
        }
        // If don't have any valid info yet, reply empty message
        if (me.infoUpdater == -1) {
          Reply(msg->replyTid, (char*)1, 0);
        } else {
          Reply(msg->replyTid, (char*)&(me.info[0].pos), sizeof(Position));
        }
        break;
      }
      case FIND_POSITION: {
        if (me.tailMode) {
          PrintDebug(me.ui, "find position while merge????");
          break;
        }

        PrintDebug(me.ui, "Train locking %d", me.trainNum);
        // Only 1 train can lock at the same time.
        lock(me.timeserver);
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
          } else if (msg->type == GET_POSITION) {
            Reply(msg->replyTid, (char*)1, 0);
          } else {
            PrintDebug(me.ui, "WARNN Drop %d", msg->type);
          }
        }
        DriverMsg dMsg;
        dMsg.type = REPORT_INFO;
        for (int i = 0; i < me.numTrainInGroup; i++) {
          Send(me.trainId[i],
              (char *)&dMsg, sizeof(DriverMsg),
              (char*)&me.info[i], sizeof(DumbDriverInfo));
        }
        me.infoUpdater = Create(3, trainNavigateNagger);
        unlock();
        break;
      }
      case MERGE_HEAD: {
        PrintDebug(me.ui, "merge head");
        if (me.tailMode) {
          PrintDebug(me.ui, "Cannot be a head when in tail mode??");
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

        PrintDebug(me.ui, "merge head done");
        break;
      }
      case MERGE_TAIL: {
        PrintDebug(me.ui, "merge tail begin");
        if (me.tailMode) {
          PrintDebug(me.ui, "Double merge tail??");
          break;
        }
        // Enters courier mode that passes dumb_train msg to 'real' controller
        me.tailMode = 1;
        me.headTid = msg->data2;
        clearReservation(me.trackManager, me.trainNum);
        Reply(msg->replyTid, (char*)1, 0);
        PrintDebug(me.ui, "merge tail done");
        break;
      }
      case SEPARATE_TAIL: {
        if (!me.tailMode) {
          PrintDebug(me.ui, "Not in tail mode..??"); break;
        }
        me.tailMode = 0;
        me.headTid = 0;
        // TODO, behaviour is not clearly defined yet.
        // reserve my own track and prediction??
        break;
      }
      case REPORT_INFO: {
        if (!me.tailMode) {
          PrintDebug(me.ui, "Report info Not in tail mode..??"); break;
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
          PrintDebug(me.ui, "Update Parent Not in tail mode..??"); break;
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
          PrintDebug(me.ui, "Reverse Speed Not in tail mode..??"); break;
        }

        for (int i = 0; i < me.numTrainInGroup; i++) {
          Send(me.trainId[i], (char *)msg, sizeof(DriverMsg), (char*)1, 0);
        }
        break;
      }
      case QUERY_STOPPING_DISTANCE: {
        if (!me.tailMode) {
          PrintDebug(me.ui, "Query stopping dist Not in tail mode..??"); break;
        }

        int stoppingDist = 0;
        // ASSUME ONLY 1 train when in tail mode.
        Send(me.trainId[0], (char*)msg, sizeof(DriverMsg),
            (char*)&stoppingDist, sizeof(int));
        // Reply the head that made query.
        Reply(me.headTid, (char*)&stoppingDist, sizeof(int));
        break;

      }
      default: {
        PrintDebug(me.ui, "Not Handled %d", msg->type);
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
