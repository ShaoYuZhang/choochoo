static void toPosition(Driver* me, Position* pos) {
  pos->landmark1.type = me->lastSensorIsTerminal ? LANDMARK_END : LANDMARK_SENSOR;
  pos->landmark1.num1 = me->lastSensorBox;
  pos->landmark1.num2 = me->lastSensorVal;
  pos->landmark2.type = me->nextSensorIsTerminal ? LANDMARK_END : LANDMARK_SENSOR;
  pos->landmark2.num1 = me->nextSensorBox;
  pos->landmark2.num2 = me->nextSensorVal;
  pos->offset = (int)me->distanceFromLastSensor;
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
    //TrainDebug(me, "Set Switch Success");
    //printLandmark(me, &setSwitch.landmark1);
    int haveNextSwitch = 0;
    for (int i = me->nextSetSwitchNode + 1; i < me->stopNode; i++) {
      if (me->route.nodes[i].landmark.type == LANDMARK_SWITCH &&
          me->route.nodes[i].landmark.num1 == BR ) {
        haveNextSwitch = 1;
        me->nextSetSwitchNode = i;
        break;
      }
    }
    updatePrediction(me);
    int reserveStatus = reserveMoreTrack(me, 0, getStoppingDistance(me)); // moving
    if (reserveStatus == RESERVE_FAIL) {
      reroute(me);
    }
    if (!haveNextSwitch) {
      me->nextSetSwitchNode = -1;
    }
  }
}

static int reserveMoreTrack(Driver* me, int stationary, int stoppingDistance) {
  // Note on passing in stopping distance:
  // if a trainSetSpeed command follows immediately after reserveMoretrack then
  // pass in the stoppingDistance of the newSpeed, else use getStoppingDistance(me)
  ReleaseOldAndReserveNewTrackMsg qMsg;
  qMsg.type = RELEASE_OLD_N_RESERVE_NEW;
  qMsg.trainNum = me->trainNum;
  qMsg.stoppingDistance = stoppingDistance;
  qMsg.lastSensor.type = me->lastSensorIsTerminal ? LANDMARK_END : LANDMARK_SENSOR;
  qMsg.lastSensor.num1 = me->lastSensorBox;
  qMsg.lastSensor.num2 = me->lastSensorVal;

  if (!stationary) {
    if (me->numPredictions > 10) {
      TrainDebug(me, "__Can't reserve >10 predictions");
    } else {
      //TrainDebug(me, "Reserving track");
      qMsg.numPredSensor = me->numPredictions;
      for(int i = 0; i < me->numPredictions; i++) {
        qMsg.predSensor[i] = me->predictions[i].sensor;
        //printLandmark(me, &qMsg.predSensor[i]);
      }
    }
  } else {
    qMsg.stoppingDistance = 1;
    qMsg.numPredSensor = 0;
  }

  int previousLandmarkState = me->reserveFailedLandmark.type;
  int len = Send(
      me->trackManager, (char*)&qMsg, sizeof(ReleaseOldAndReserveNewTrackMsg),
      (char*)&(me->reserveFailedLandmark), sizeof(TrackLandmark));
  if (len > 0) {
    //TrainDebug(me, "Failed cuz couldn't get landmark");
    printLandmark(me, &me->reserveFailedLandmark);
    return RESERVE_FAIL;
  } else if (!stationary &&
      previousLandmarkState != LANDMARK_BAD &&
      me->reserveFailedLandmark.type != LANDMARK_BAD){
    //TrainDebug(me, "Got landmark bad.");
    me->reserveFailedLandmark.type = LANDMARK_BAD;
  }
  return RESERVE_SUCESS;
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
  for (int i = 0; i < trackMsg.numPred; i++) {
    TrackSensorPrediction prediction = trackMsg.predictions[i];
  }

  TrackSensorPrediction primaryPrediction = trackMsg.predictions[0];
  me->distanceToNextSensor = primaryPrediction.dist;
  if (primaryPrediction.sensor.type != LANDMARK_SENSOR &&
      primaryPrediction.sensor.type != LANDMARK_END) {
    TrainDebug(me, "QUERY_NEXT_SENSOR_FROM_SENSOR ..bad %d", primaryPrediction.sensor.type);
  }
  me->nextSensorIsTerminal = (primaryPrediction.sensor.type == LANDMARK_END);

  if (primaryPrediction.sensor.num2 != 0) {
    me->nextSensorBox = primaryPrediction.sensor.num1;
    me->nextSensorVal = primaryPrediction.sensor.num2;
  } else {
    //TrainDebug(me, "No prediction.. has position");
    //printLandmark(me, &qMsg.position1.landmark1);
    //printLandmark(me, &qMsg.position1.landmark2);
    //TrainDebug(me, "Offset %d", qMsg.position1.offset);
  }
  sendUiReport(me);
}

static void getRoute(Driver* me, DriverMsg* msg) {
  //TrainDebug(me, "Getting Route.");
  TrackMsg trackmsg;
  if (me->testMode) {
    //TrainDebug(me, "Test Mode");
    trackmsg.type = GET_PRESET_ROUTE;
    trackmsg.data = msg->data3; // an index to a preset route
  } else {
    trackmsg.type = ROUTE_PLANNING;
    trackmsg.landmark1 = me->reserveFailedLandmark;
    toPosition(me, &trackmsg.position1);
    //printLandmark(me, &trackmsg.position1.landmark1);
    //printLandmark(me, &trackmsg.position1.landmark2);
    //TrainDebug(me, "Offset %d", trackmsg.position1.offset);

    trackmsg.position2 = msg->pos;
    trackmsg.data = (char)me->trainNum;
  }

  Send(me->trackManager, (char*)&trackmsg,
      sizeof(TrackMsg), (char*)&(me->route), sizeof(Route));
  me->routeRemaining = 0;
  me->previousStopNode = 0;
  me->distanceFromLastSensorAtPreviousStopNode = me->distanceFromLastSensor;
  me->stopSensorVal = -1;
  me->stopSensorBox = -1;
  if (!me->testMode) {
    printRoute(me);
  }
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
      //TrainDebug(me, "Navi Nagger. %d", d);
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

static void updateStopNode(Driver* me) {
  // Find the first reverse on the path, stop if possible.
  me->stopNode = me->route.length-1;
  const int stoppingDistance =
    interpolateStoppingDistance(me,
        getVelocity(me));
  int stop = stoppingDistance;
  for (int i = me->previousStopNode; i < me->route.length-1; i++) {
    if (me->route.nodes[i].num == REVERSE) {
      me->stopNode = i;
      break;
    }
  }
  //TrainDebug(me, "UpdateStop. Node:%d %dmm",
  //    me->stopNode, stoppingDistance);

  // Find the stopping distance for the stopNode.
  // S------L------L---|-----L---------R------F
  //                   |__stop_dist____|
  // |__travel_dist____|
  // |delay this much..|
  //TrainDebug(me, "Need %d mm at StopNode %d", stop, me->stopNode-1);
  if (stoppingDistance == 0) {
    return;
  }
  for (int i = me->stopNode-1; i >= me->previousStopNode; i--) {
    // Minus afterwards so that stoppping distance can be zero.
    if (stop > 0) {
      stop -= me->route.nodes[i].dist;
      //TrainDebug(me, "Stop %d %d", stop, i);
    }

    if (stop <= 0) {
      int previousStop = -stop;
      //TrainDebug(me, "PreviousStop %d", previousStop);
      me->stopSensorBox = -1;
      me->stopSensorVal = -1;

      // Find previous sensor.
      for (int j = i; j >= me->previousStopNode; j--) {
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
        previousStop += (int)me->distanceFromLastSensorAtPreviousStopNode;
        //TrainDebug(me, "Use Last sensor now");
      }
      // Else..
      //     |---stopSensorBox
      // =*==S======S====F===S
      //     |__  |_stop_|____stopping distance
      //       |__|
      //          |______ previous stop
      me->distancePassStopSensorToStop = previousStop;
      //TrainDebug(me, "Stop Sensor %c%d",
      //    'A'+me->stopSensorBox, me->stopSensorVal);
      //TrainDebug(me, "Stop %dmm after sensor", previousStop);
      break;
    }
  }
  if (stop > 0) {
      //TrainDebug(me, "No room to stop??? %d", stop);
      //TrainDebug(me, "StopNode-1 %d, remaining %d ", me->stopNode-1,
      //    me->previousStopNode);

      me->stopNow = 1;
  }

  //TrainDebug(me, "Finish update stop %d %d %d", me->stopNode,
  //(me->stopNode == (me->route.length-1)), (me->route.nodes[me->stopNode].num == REVERSE));
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
      //TrainDebug(me, "Triggered expected sensor!! %d", val);
      me->routeRemaining = i;
      break;
    }
  }

  // TODO if stoppped, update next stopNode.
}

static void updateSetSwitch(Driver* me) {
  for (int i = me->routeRemaining; i < me->stopNode; i++) {
    if (me->route.nodes[i].landmark.type == LANDMARK_SWITCH &&
        me->route.nodes[i].landmark.num1 == BR && me->nextSetSwitchNode == -1) {
      //TrainDebug(me, "Will try to set:");
      //printLandmark(me, &(me->route.nodes[i].landmark));
      me->nextSetSwitchNode = i;
      //TrainDebug(me, "At: %d", i);
    }
  }
}
static void QueryNextSensor(Driver* me, TrackNextSensorMsg* trackMsg) {
  TrackMsg qMsg;
  qMsg.type = QUERY_NEXT_SENSOR_FROM_SENSOR;
  qMsg.landmark1.type = LANDMARK_SENSOR;
  qMsg.landmark1.num1 = me->lastSensorBox;
  qMsg.landmark1.num2 = me->lastSensorVal;
  Send(me->trackManager, (char*)&qMsg, sizeof(TrackMsg),
      (char*)trackMsg, sizeof(TrackNextSensorMsg));
}

static int QueryIsSensorReserved(Driver* me, int box, int val) {
  char isReserved = 0;
  TrackMsg qMsg;
  qMsg.type = QUERY_SENSOR_RESERVED;
  qMsg.data = me->trainNum;
  qMsg.landmark1.type = LANDMARK_SENSOR;
  qMsg.landmark1.num1 = box;
  qMsg.landmark1.num2 = val;
  Send(me->trackManager, (char*)&qMsg, sizeof(TrackMsg),
      (char*)&isReserved, 1);
  return (int)isReserved;
}

static void printLandmark(Driver* me, TrackLandmark* l) {
  if (l->type == LANDMARK_SENSOR) {
    TrainDebug(me, "Landmark Sn  %c%d",
        'A' +l->num1, l->num2);
  } else if (l->type == LANDMARK_END) {
    TrainDebug(me, "Landmark %s %d",
        l->num1 == EN ? "EN" : "EX",
        l->num2);
  } else if (l->type == LANDMARK_FAKE) {
    TrainDebug(me, "Landmark Fake %d %d",
        l->num1, l->num2);
  } else if (l->type == LANDMARK_SWITCH) {
    TrainDebug(me, "Landmark Switch Num:%d Type:%s",
        l->num2, l->num1 == MR ? "MR" : "BR");
  } else if (l->type == LANDMARK_BAD) {
    //TrainDebug(me, "Landmark type: bad.");
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
static void printRoute(Driver* me) {
  TrainDebug(me, "Route Distance %d", me->route.dist);

  TrainDebug(me, "<Route>");
  for (int i = 0; i < me->route.length; i++) {
    RouteNode node = me->route.nodes[i];
    if (node.num == -1) {
      TrainDebug(me, "%d reverse", i);
    } else {
      if (node.landmark.type == LANDMARK_SENSOR) {
        TrainDebug(me, "%d Landmark Sn  %c%d D:%d",
            i, 'A' +node.landmark.num1, node.landmark.num2, node.dist);
      } else if (node.landmark.type == LANDMARK_END) {
        TrainDebug(me, "%d Landmark %s %d D:%d",
            i, node.landmark.num1 == EN ? "EN" : "EX",
            node.landmark.num2, node.dist);
      } else if (node.landmark.type == LANDMARK_FAKE) {
        TrainDebug(me, "%d Landmark Fake %d %d D:%d",
            i, node.landmark.num1, node.landmark.num2, node.dist);
      }

      if (node.landmark.type == LANDMARK_SWITCH && node.landmark.num1 == BR) {
        TrainDebug(me, "%d Switch %d to %s Type:%d D:%d", i, node.landmark.num2,
            node.num == SWITCH_CURVED ? "Curve" : "Straight", node.landmark.num1,
            node.dist);
      }
    }
  }
  TrainDebug(me, "</Route>");

}

static void setRoute(Driver* me, DriverMsg* msg) {
  //TrainDebug(me, "Route setting!");
  me->stopCommited = 0;

  getRoute(me, msg);
  if (me->route.length != 0) {
    if (me->route.nodes[1].num == REVERSE) {
      // Don't need to reverse... cuz.. it's probably stuck.
      trainSetSpeed(msg->data2, 0, 0, me);
      updateStopNode(me);
      me->nextSetSwitchNode = -1;
      updateSetSwitch(me);
    } else {
      int reserveStatus = reserveMoreTrack(me, 0, me->d[(int)msg->data2][ACCELERATE][MAX_VAL]); // Moving
      if (reserveStatus == RESERVE_SUCESS) {
        trainSetSpeed(msg->data2, 0, 0, me);
        updateStopNode(me);
        me->nextSetSwitchNode = -1;
        updateSetSwitch(me);
      } else {
        TrainDebug(me, "Cannot reserve track!");
        trainSetSpeed(0, 0, 0, me);
        me->rerouteCountdown = 200;
      }
    }
  } else {
    TrainDebug(me, "No route found!");
    trainSetSpeed(0, 0, 0, me);
    me->stopCommited = 1;
  }
}

#include "DriverHelperTask.c"
