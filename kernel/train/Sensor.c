#include <Sensor.h>
#include <IoServer.h>
#include <IoHelper.h>
#include <syscall.h>
#include <TimeServer.h>
#include <NameServer.h>

static int CURR_SENSOR_BOX;
static int CURR_HIGH_BITS;
static int SENSOR_VALUE[NUM_SENSOR_BOX][16];
static int IGNORE_RESULT;

static int sensorSubscriber[10];
static int subscriberIndex;
static int subscriberUpdateIndex;

static Sensor sensorBuffer[10];
static int sensorBufferHead;
static int sensorBufferTail;

static char responseBuffer[10];
static int responseIndex;

static void sensorQueryWorker() {
  char com1Name[] = IOSERVERCOM1_NAME;
  int com1 = WhoIs(com1Name);

  int parent = MyParentsTid();

  for (;;) {
    SensorMsg msg;
    msg.type = QUERY_WORKER;
    Send(parent, (char *)&msg, sizeof(SensorMsg), (char *)NULL, 0);
    Putc(com1, 133);
  }
}

static void sensorQueryResponseWorker() {
  char com1Name[] = IOSERVERCOM1_NAME;
  int com1 = WhoIs(com1Name);
  char timeName[] = TIMESERVER_NAME;
  int time = WhoIs(timeName);

  int parent = MyParentsTid();

  for (;;) {
    SensorMsg msg;
    char c = Getc(com1);
    msg.type = QUERY_RESPONSE_WORKER;
    msg.data = c;
    msg.time = Time(time);
    Send(parent, (char *)&msg, sizeof(SensorMsg), (char *)NULL, 0);
  }
}

static void sensorQueryTimeoutWorker() {
  char timeServerName[] = TIMESERVER_NAME;
  int timeServer= WhoIs(timeServerName);

  int parent = MyParentsTid();

  for (;;) {
    SensorMsg msg;
    msg.type = QUERY_TIMEOUT_WORKER;
    Send(parent, (char *)&msg, sizeof(SensorMsg), (char *)NULL, 0);
    Delay(10, timeServer);
  }
}

static void sensorQueryResponseTimeoutWorker() {
  char timeServerName[] = TIMESERVER_NAME;
  int timeServer= WhoIs(timeServerName);

  int parent = MyParentsTid();

  for (;;) {
    SensorMsg msg;
    msg.type = QUERY_RESPONSE_TIMEOUT_WORKER;
    Send(parent, (char *)&msg, sizeof(SensorMsg), (char *)NULL, 0);
    Delay(7, timeServer);
  }
}

static void sensorCourier() {
  int parent = MyParentsTid();
  for (;;) {
    SensorMsg msg;
    msg.type = SENSOR_COURIER;
    SensorWorkUnit work;
    Send(parent, (char *)&msg, sizeof(SensorMsg),
        (char *)&work, sizeof(SensorWorkUnit));

    int tid = work.tid;
    Sensor s = work.sensor;

    Send(tid, (char *)&s, sizeof(Sensor), (char *)NULL, 0);
  }
}

static unsigned int increment_offset( int curr_offset ) {
  curr_offset++;
  if (curr_offset == 10) {
    curr_offset = 0;
  }
  return curr_offset;
}

static void add_to_buffer(Sensor s) {
  unsigned int temp_new_head = increment_offset(sensorBufferHead);
  if (temp_new_head != sensorBufferTail) {
    sensorBuffer[sensorBufferHead] = s;
    sensorBufferHead = temp_new_head;
  }
}

static int buffer_empty() {
  return sensorBufferHead == sensorBufferTail;
}

static Sensor remove_from_buffer() {
  if (sensorBufferHead != sensorBufferTail) {
    Sensor s = sensorBuffer[sensorBufferTail];
    sensorBufferTail = increment_offset(sensorBufferTail);
    return s;
  } else {
    Sensor s;
    return s;
  }
}

static void sensorResponded(char response, int time) {
    if (!IGNORE_RESULT) {
      for (int i = 0; i < 8; i++) {
          int mask = 1 << i;
          int offset = 0;
          if (CURR_HIGH_BITS) {
              offset += 8;
          }
          if ((mask & response) != 0 && SENSOR_VALUE[CURR_SENSOR_BOX][7-i+offset] == 0) {
            Sensor s;
            s.box = CURR_SENSOR_BOX;
            s.val = 8 -i + offset;
            s.time = time;

            add_to_buffer(s);
          }
          SENSOR_VALUE[CURR_SENSOR_BOX][7-i+offset] = ((mask & response) !=0);
      }
    }
    if (CURR_HIGH_BITS) {
        CURR_HIGH_BITS = 0;
        ++CURR_SENSOR_BOX;
    } else {
        CURR_HIGH_BITS = 1;
    }
    if (CURR_SENSOR_BOX >= NUM_SENSOR_BOX) {
        CURR_SENSOR_BOX = 0;
        IGNORE_RESULT = 0;
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
  responseIndex = 0;
  for (i = 0; i < NUM_SENSOR_BOX; ++i) {
    for (j = 0; j < 16; ++j) {
      SENSOR_VALUE[i][j] = 0;
    }
  }

  subscriberIndex = 0;
  subscriberUpdateIndex = 0;
  sensorBufferHead = 0;
  sensorBufferTail = 0;

  int queryWorker = Create(7, sensorQueryWorker);
  Create(8, sensorQueryResponseWorker);
  int queryTimeoutWorker = Create(7, sensorQueryTimeoutWorker);
  int queryResponseTimeoutWorker = Create(7, sensorQueryResponseTimeoutWorker);
  int courier  = Create(7, sensorCourier);

  int queryWorkerReady = 0;
  int queryTimeout = 0;
  int queryResponseTimeout = 0;
  int startQueryResponseTimeout = 0;
  int courierReady = 0;

  char timerName[] = TIMESERVER_NAME;
  int timer = WhoIs(timerName);

  IGNORE_RESULT = 1;

  // sensor server is time sensitive, it delays and tries
  // to avoid the initialization period where there are
  // a lot of chars in com1 buffer
  Delay(700, timer);

  // Clear sensor memory after reading.
  Putc(com1, 192);
  // end init

  for ( ;; ) {
    if (queryTimeout && queryWorkerReady) {
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
        if (responseIndex == 0) {
          if (queryResponseTimeout) {
            queryResponseTimeout = 0;
            Reply(queryResponseTimeoutWorker, (char *)NULL, 0);
          } else {
            startQueryResponseTimeout = 1;
          }
        }
        responseBuffer[responseIndex++] = response;
        if (responseIndex == 10) {
          for (int i = 0; i < 10; i++) {
            sensorResponded(responseBuffer[i], msg.time);
          }
          responseIndex = 0;
        }
        break;
      }
      case QUERY_TIMEOUT_WORKER: {
        queryTimeout = 1;
        break;
      }
      case QUERY_RESPONSE_TIMEOUT_WORKER: {
        if (startQueryResponseTimeout) {
          startQueryResponseTimeout = 0;
          Reply(queryResponseTimeoutWorker, (char *)NULL, 0);
        } else {
          queryResponseTimeout = 1;
          responseIndex = 0;
          CURR_SENSOR_BOX = 0;
          CURR_HIGH_BITS = 0;
        }
        break;
      }
      case QUERY_RECENT: {
        sensorSubscriber[subscriberIndex++] = tid;
        Reply(tid, (char *)NULL, 0);
        break;
      }
      case SENSOR_COURIER: {
        courierReady = 1;
        break;
      }
      default: {
        ASSERT(FALSE, "invalid sensor msg type.");
      }
    }

    if (courierReady && !buffer_empty() && subscriberIndex != 0) {
      Sensor s = sensorBuffer[sensorBufferTail];

      SensorWorkUnit work;
      work.sensor = s;
      work.tid = sensorSubscriber[subscriberUpdateIndex++];

      Reply(courier, (char *)&work, sizeof(SensorWorkUnit));
      if (subscriberUpdateIndex == subscriberIndex) {
        subscriberUpdateIndex = 0;
        remove_from_buffer();
      }
    }
  }
}

int startSensorServerTask() {
  return Create(3, sensorServer);
}
