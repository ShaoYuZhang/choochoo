#include <Sensor.h>
#include <IoServer.h>
#include <IoHelper.h>
#include <syscall.h>
#include <TimeServer.h>
#include <NameServer.h>

static int CURR_SENSOR_BOX;
static int CURR_HIGH_BITS;
static int SENSOR_QUERY_FINISHED;
static int SENSOR_VALUE[NUM_SENSOR_BOX][16];
// TODO, need to be a list of subscriber later
static int sensorUpdateSubscriber = -1;

static void sensorQueryWorker() {
  char com1Name[] = IOSERVERCOM1_NAME;
  int com1 = WhoIs(com1Name);

  int parent = MyParentsTid();

  for (;;) {
    SensorMsg msg;
    msg.type = QUERY_WORKER;
    Send (parent, (char *)&msg, sizeof(SensorMsg), (char *)NULL, 0);

    Putc( com1, 133);
  }
}

static void sensorQueryResponseWorker() {
  char com1Name[] = IOSERVERCOM1_NAME;
  int com1 = WhoIs(com1Name);

  int parent = MyParentsTid();

  for (;;) {
    SensorMsg msg;
    char c = Getc(com1);
    msg.type = QUERY_RESPONSE_WORKER;
    msg.data = c;
    Send (parent, (char *)&msg, sizeof(SensorMsg), (char *)NULL, 0);
  }
}

static void sensorQueryTimeoutWorker() {
  char timeServerName[] = TIMESERVER_NAME;
  int timeServer= WhoIs(timeServerName);

  int parent = MyParentsTid();

  for (;;) {
    SensorMsg msg;
    msg.type = QUERY_TIMEOUT_WORKER;
    Send (parent, (char *)&msg, sizeof(SensorMsg), (char *)NULL, 0);
    // TODO (cao) need tuning
    Delay(10, timeServer);
  }
}

static void sensorResponded(char response) {
    if (SENSOR_QUERY_FINISHED) {
        return;
    }
    int i;
    for (i = 0; i < 8; i++) {
        int mask = 1 << i;
        int offset = 0;
        if (CURR_HIGH_BITS) {
            offset += 8;
        }
        if ((mask & response) != 0 && SENSOR_VALUE[CURR_SENSOR_BOX][7-i+offset] == 0) {
          // sensor triggered, update subscriber
          if (sensorUpdateSubscriber != -1) {
            Sensor s;
            s.box = CURR_SENSOR_BOX;
            s.val = 8 -i + offset;

            Reply(sensorUpdateSubscriber, (char *)&s, sizeof(Sensor));
          }
        }
        SENSOR_VALUE[CURR_SENSOR_BOX][7-i+offset] = ((mask & response) !=0);
    }
    if (CURR_HIGH_BITS) {
        CURR_HIGH_BITS = 0;
        ++CURR_SENSOR_BOX;
    } else {
        CURR_HIGH_BITS = 1;
    }
    if (CURR_SENSOR_BOX >= NUM_SENSOR_BOX) {
        CURR_SENSOR_BOX = 0;
        SENSOR_QUERY_FINISHED = 1;
    }
}

static void sensorServer() {
  char com1Name[] = IOSERVERCOM1_NAME;
  int com1 = WhoIs(com1Name);

  char sensorName[] = SENSOR_NAME;
  RegisterAs(sensorName);

  int i, j;
  CURR_SENSOR_BOX = 0;
  CURR_HIGH_BITS = 0;
  SENSOR_QUERY_FINISHED = 1;
  for (i = 0; i < NUM_SENSOR_BOX; ++i) {
    for (j = 0; j < 16; ++j) {
      SENSOR_VALUE[i][j] = 0;
    }
  }

  Putc(com1, 192);

  int queryWorker = Create(7, sensorQueryWorker);
  Create(8, sensorQueryResponseWorker);
  int queryTimeoutWorker = Create(7, sensorQueryTimeoutWorker);

  int queryWorkerReady = 0;
  int queryTimeout = 0;

  // end init

  for ( ;; ) {
    // TODO (cao), pulling every x ms vs  pulling everytimes query is done??
    if (queryTimeout && queryWorkerReady) {
      SENSOR_QUERY_FINISHED = 0;
      queryTimeout = 0;
      queryWorkerReady = 0;
      Reply(queryWorker, (char *)NULL, 0);
      Reply(queryTimeoutWorker, (char *)NULL, 0);
    }

    int tid = -1;
    SensorMsg msg;
    Receive(&tid, (char*)&msg, sizeof(SensorMsg));
    switch (msg.type) {
      case QUERY_WORKER: {
        queryWorkerReady = 1;
        break;
      }
      case QUERY_RESPONSE_WORKER: {
        Reply(tid, (char *)NULL, 0);
        char response = msg.data;
        sensorResponded(response);
        break;
      }
      case QUERY_TIMEOUT_WORKER: {
        queryTimeout = 1;
        break;
      }
      case QUERY_RECENT: {
        sensorUpdateSubscriber = tid;
        break;
      }
    }
  }
}



int startSensorServerTask() {
  return Create(3, sensorServer);
}
