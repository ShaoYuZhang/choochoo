#define NAGGER_TICK 2

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

// Unlike the delayer above, this one handles stop commands
// as opposed to reverse commands
static void trainStopDelayer() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = STOP_DELAYER;
  for (;;) {
    int stopTime = 0;
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)&stopTime, 4);
    int numTick = stopTime / 10;
    Delay(numTick, timeserver);
  }
}

static void trainNavigateNagger() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = NAVIGATE_NAGGER;
  for (;;) {
    Delay(NAGGER_TICK, timeserver);
    msg.timestamp = Time(timeserver) * 10;
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}


