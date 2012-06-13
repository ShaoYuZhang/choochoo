#include "Train.h"
#include <IoServer.h>
#include <ts7200.h>
#include <util.h>
#include <TimeServer.h>
#include <NameServer.h>
#include <IoHelper.h>

#define NUM_WORKER 4
#define WORKER_SIG 0xff

static int switchStatus[NUM_SWITCHES];

typedef struct Train {
  int speed;
} Train;

static int com1;
static Train train[NUM_TRAINS];
// Workers that is responsible for reversing train.
static int worker[NUM_WORKER];
static int numWorkerLeft;

// Responsider for delag msg server about positive train speed
void trainWorker() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);

  TrainMsg msg;
  for (;;) {
    int tid = -1;
    Receive(&tid, (char*)&msg, sizeof(TrainMsg));
    Reply(tid, (char*)1, 0);
    // delay
    int numTick = msg.data3; // num of 10ms
    Delay(numTick, timeserver);
    // Send positive speed
    msg.data3 = WORKER_SIG;
    Send(tid, (char*)&msg, sizeof(TrainMsg), (char*)&msg, 0);
  }
}

void trainController() {
  char com1Name[] = IOSERVERCOM1_NAME;
  com1 = WhoIs(com1Name);
  char trainName[] = TRAIN_NAME;
  RegisterAs(trainName);

  for (int i = 0; i < NUM_TRAINS; i++) {
    train[i].speed = 0;
  }

  numWorkerLeft = NUM_WORKER-1;
  for (int i = 0; i < NUM_WORKER; i++) {
    worker[i] = Create(1, trainWorker);
  }

  for (;;) {
    int tid = -1;
    TrainMsg msg;
    Receive(&tid, (char*)&msg, sizeof(TrainMsg));

    switch (msg.type) {
      case GET_SWITCH: {
        Reply(tid, (char*)(switchStatus + msg.data1), 4);
        break;
      }
      case SET_SWITCH: {
        Reply(tid, (char*)1, 0);
        trainSetSwitch((int)msg.data1, (int)msg.data2);
        break;
      }
      case GET_SPEED: {
        const int trainNum = msg.data1;
        Reply(tid, (char*)&(train[trainNum].speed), 4);
        break;
      }
      case SET_SPEED: {
        Reply(tid, (char*)1, 0);
        trainSetSpeed(&msg);
        break;
      }
      default: {
        ASSERT(FALSE, "Not suppported train message type.");
      }
    }

  }
}

void trainSetSwitch(int sw, int state) {
  char msg[3];
  msg[0] = (char)state;
  msg[1] = (char)sw;
  msg[2] = 0;

  Putstr(com1, msg, 3);
  switchStatus[sw] = state;
}

void trainSetSpeed(TrainMsg* origMsg) {
  const int trainNum = origMsg->data1;
  const int speed = origMsg->data2;
  if (origMsg->data3 == WORKER_SIG) {
    numWorkerLeft++;
    ASSERT(speed > 0, "Train worker has negative speed.");
  }

  char msg[3];
  msg[1] = (char)trainNum;
  msg[2] = 0;
  if (speed >= 0) {
    msg[0] = (char)speed;
  } else {
    msg[0] = 0;
    origMsg->data2 = train[trainNum].speed * -1;
    origMsg->data3 = 250; // 2.5s . TODO, calculate from train speed.
    Send(worker[numWorkerLeft--], (char*)origMsg,
        sizeof(TrainMsg), (char*)origMsg, 0);
    ASSERT(numWorkerLeft < -1, "Used non-existence worker id");
  }

  Putstr(com1, msg, 3);
  train[trainNum].speed = msg[1];
}

int startTrainControllerTask() {
  return Create(2, trainController);
}
