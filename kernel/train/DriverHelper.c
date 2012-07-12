
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
    Delay(2, timeserver); // .15 seconds
    msg.timestamp = Time(timeserver) * 10;
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

static void printRoute(Driver* me) {
  TrainDebug(me, "Route Distance %d", me->route.dist);
  TrainDebug(me, "Num Node %d", me->route.length);

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
        TrainDebug(me, "%d Set switch %d %s Type:%d", i, node.landmark.num2,
            node.num == SWITCH_CURVED ? "Curve" : "Straight", node.landmark.num1);
      }
    }
  }
  TrainDebug(me, "</Route>");

}

static void sendUiReport(Driver* me) {
  me->uiMsg.velocity = getVelocity(me) / 100;
  me->uiMsg.lastSensorUnexpected     = me->lastSensorUnexpected;
  me->uiMsg.lastSensorBox            = me->lastSensorBox;
  me->uiMsg.lastSensorVal            = me->lastSensorVal;
  me->uiMsg.lastSensorActualTime     = me->lastSensorActualTime;
  me->uiMsg.lastSensorPredictedTime  = me->lastSensorPredictedTime;
  me->uiMsg.lastSensorIsTerminal     = me->lastSensorIsTerminal;

  me->uiMsg.speed                    = me->speed;      // 0 - 14
  me->uiMsg.speedDir                 = me->speedDir;
  me->uiMsg.distanceFromLastSensor   = (int)me->distanceFromLastSensor;
  me->uiMsg.distanceToNextSensor     = (int)me->distanceToNextSensor;

  me->uiMsg.nextSensorBox            = me->nextSensorBox;
  me->uiMsg.nextSensorVal            = me->nextSensorVal;
  me->uiMsg.nextSensorPredictedTime  = me->nextSensorPredictedTime;
  me->uiMsg.nextSensorIsTerminal     = me->nextSensorIsTerminal;

  me->uiMsg.lastSensorDistanceError  = me->lastSensorDistanceError;

  Send(me->ui, (char*)&(me->uiMsg), sizeof(TrainUiMsg), (char*)1, 0);
}

static void setRoute(Driver* me, DriverMsg* msg) {
  TrainDebug(me, "Route setting!");
  me->stopCommited = 0;

  getRoute(me, msg);
  if (me->route.length != 0) {
    if (me->route.nodes[0].dist == 0 && me->route.nodes[1].num == REVERSE) {
      me->stopNode = 1;
      me->previousStopNode = 1;
      me->distanceFromLastSensorAtPreviousStopNode = me->distanceFromLastSensor;
      me->speedAfterReverse = msg->data2;
      trainSetSpeed(-1, getStoppingTime(me), 0, me);
    } else {
      int reserveStatus = reserveMoreTrack(me);
      if (reserveStatus == RESERVE_SUCESS) {
        trainSetSpeed(msg->data2, 0, 0, me);
        updateStopNode(me);
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
  }
}
