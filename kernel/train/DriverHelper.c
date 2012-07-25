static void trySetSwitch_and_getNextSwitch(MultiTrainDriver* me) {
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

    if (!haveNextSwitch) {
      me->nextSetSwitchNode = -1;
    }
  }
}

static void updatePrediction(MultiTrainDriver* me) {
  DriverMsg dMsg;
  dMsg.type = UPDATE_PARENT_ABOUT_PREDICTION;
  for (int i = 0; i < MAX_TRAIN_IN_GROUP; i++) {
    Send(me->trainId[i], (char *)&dMsg, sizeof(DriverMsg), (char *)NULL, 0);
  }
}

static void getRoute(MultiTrainDriver* me, Position* from, DriverMsg* msg) {
  PrintDebug(me->ui, "%d Getting Route.", me->trainNum);
  TrackMsg trackmsg;
  if (me->testMode) {
    //TrainDebug(me, "Test Mode");
    trackmsg.type = GET_PRESET_ROUTE;
    trackmsg.data = msg->data3; // an index to a preset route
  } else {
    trackmsg.type = ROUTE_PLANNING;

    trackmsg.position1 = *from;
    trackmsg.position2 = msg->pos;
    trackmsg.trainNum = (char)me->trainNum;
    trackmsg.data = msg->data3; // May be ONE_PATH_DEST.

    printLandmark(me, &trackmsg.position1.landmark1);
    printLandmark(me, &trackmsg.position1.landmark2);
    PrintDebug(me->ui, "Offset %d", trackmsg.position1.offset);
  }

  Send(me->trackManager, (char*)&trackmsg,
      sizeof(TrackMsg), (char*)&(me->route), sizeof(Route));
  me->routeRemaining = 0;
  me->previousStopNode = 0;
  me->distanceFromLastSensorAtPreviousStopNode = me->info[0].pos.offset;
  me->stopSensorVal = -1;
  me->stopSensorBox = -1;
  if (!me->testMode) {
    printRoute(me);
  }
}

static int shouldStopNow(MultiTrainDriver* me) {
  if (me->stopNow) {
    return 2; // no room to stop, must stop now
  }
  if ( me->info[0].pos.landmark1.num1 == me->stopSensorBox &&
        me->info[0].pos.landmark1.num2 == me->stopSensorVal) {
    me->stopSensorHit = 1;
  }
  int canUseLastSensor =
      ( me->info[0].pos.landmark1.num1 == me->stopSensorBox &&
        me->info[0].pos.landmark1.num2 == me->stopSensorVal
      ) || me->useLastSensorNow;

  if (canUseLastSensor) {
    int d = me->distancePassStopSensorToStop - (int)me->info[0].pos.offset;

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

static void updateStopNode(MultiTrainDriver* me) {
  // Find the first reverse on the path, stop if possible.
  me->stopNode = me->route.length-1;
  const int stoppingDistance = me->info[0].currentStoppingDistance;
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
  int stop = stoppingDistance;
  if (stoppingDistance == 0) {
    stop = 1; //hack
  }
  for (int i = me->stopNode-1; i >= me->previousStopNode; i--) {
    // Minus afterwards so that stoppping distance can be zero.
    if (stop > 0) {
      stop -= me->route.nodes[i].dist;
      //PrintDebug(me->ui, "Stop %d %d", stop, i);
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
        //PrintDebug(me->ui, "Use Last sensor now");
      }
      // Else..
      //     |---stopSensorBox
      // =*==S======S====F===S
      //     |__  |_stop_|____stopping distance
      //       |__|
      //          |______ previous stop
      me->distancePassStopSensorToStop = previousStop;
      //PrintDebug(me->ui, "Stop Sensor %c%d",
      //    'A'+me->stopSensorBox, me->stopSensorVal);
      //PrintDebug(me->ui, "Stop %dmm after sensor", previousStop);
      break;
    }
  }
  if (stop > 0) {
      //PrintDebug(me->ui, "No room to stop??? %d", stop);
      //PrintDebug(me->ui, "StopNode-1 %d, remaining %d ", me->stopNode-1,
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
static void updateRoute(MultiTrainDriver* me, char box, char val) {
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

static void updateSetSwitch(MultiTrainDriver* me) {
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

static int QueryIsSensorReserved(MultiTrainDriver* me, int box, int val) {
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

static void printLandmark(MultiTrainDriver* me, TrackLandmark* l) {
  if (l->type == LANDMARK_SENSOR) {
    PrintDebug(me->ui, "Landmark Sn  %c%d",
        'A' +l->num1, l->num2);
  } else if (l->type == LANDMARK_END) {
    PrintDebug(me->ui, "Landmark %s %d",
        l->num1 == EN ? "EN" : "EX",
        l->num2);
  } else if (l->type == LANDMARK_FAKE) {
    PrintDebug(me->ui, "Landmark Fake %d %d",
        l->num1, l->num2);
  } else if (l->type == LANDMARK_SWITCH) {
    PrintDebug(me->ui, "Landmark Switch Num:%d Type:%s",
        l->num2, l->num1 == MR ? "MR" : "BR");
  } else if (l->type == LANDMARK_BAD) {
    PrintDebug(me->ui, "Landmark type: bad.");
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

static void printRoute(MultiTrainDriver* me) {
  PrintDebug(me->ui, "Route Distance %d", me->route.dist);

  PrintDebug(me->ui, "<Route>");
  for (int i = 0; i < me->route.length; i++) {
    RouteNode node = me->route.nodes[i];
    if (node.num == -1) {
      PrintDebug(me->ui, "%d reverse", i);
    } else {
      if (node.landmark.type == LANDMARK_SENSOR) {
        PrintDebug(me->ui, "%d Landmark Sn  %c%d D:%d",
            i, 'A' +node.landmark.num1, node.landmark.num2, node.dist);
      } else if (node.landmark.type == LANDMARK_END) {
        PrintDebug(me->ui, "%d Landmark %s %d D:%d",
            i, node.landmark.num1 == EN ? "EN" : "EX",
            node.landmark.num2, node.dist);
      } else if (node.landmark.type == LANDMARK_FAKE) {
        PrintDebug(me->ui, "%d Landmark Fake %d %d D:%d",
            i, node.landmark.num1, node.landmark.num2, node.dist);
      }

      if (node.landmark.type == LANDMARK_SWITCH && node.landmark.num1 == BR) {
        PrintDebug(me->ui, "%d Switch %d to %s Type:%d D:%d", i, node.landmark.num2,
            node.num == SWITCH_CURVED ? "Curve" : "Straight", node.landmark.num1,
            node.dist);
      }
    }
  }
  PrintDebug(me->ui, "</Route>");

}

static void setRoute(MultiTrainDriver* me, Position* from, DriverMsg* msg) {
  //TrainDebug(me, "Route setting!");
  me->stopCommited = 0;

  getRoute(me, from, msg);
  if (me->route.length != 0) {
    if (me->route.nodes[1].num == REVERSE) {
      // Don't need to reserve... cuz.. it's probably stuck.
      groupSetSpeed(me, msg->data2);
      updateStopNode(me);
      me->nextSetSwitchNode = -1;
      updateSetSwitch(me);
    } else {
      int setSpeedSuccess= groupSetSpeed(me, msg->data2);
      if (setSpeedSuccess) {
        updateStopNode(me);
        me->nextSetSwitchNode = -1;
        updateSetSwitch(me);
      } else {
        PrintDebug(me->ui, "Cannot reserve track!");
        groupSetSpeed(me, 0);
        me->rerouteCountdown = 200;
      }
    }
  } else {
    PrintDebug(me->ui, "No route found!");
    groupSetSpeed(me, 0);
    me->stopCommited = 1;
  }
}

#include "DriverHelperTask.c"
