
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


