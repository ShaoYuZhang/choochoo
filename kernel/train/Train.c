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

static int com2;
void trainController() {
  char com1Name[] = IOSERVERCOM1_NAME;
  char com2Name[] = IOSERVERCOM2_NAME;
  com1 = WhoIs(com1Name);
  com2 = WhoIs(com2Name);
  char trainName[] = TRAIN_NAME;
  RegisterAs(trainName);

  for (int i = 0; i < NUM_TRAINS; i++) {
    train[i].speed = 0;
  }

  int numWorkerLeft = NUM_WORKER-1;
  for (int i = 0; i < NUM_WORKER; i++) {
    worker[i] = Create(1, trainWorker);
  }
  Putc(com2, 'a' + numWorkerLeft);

  for (int i = 1; i < 19; i++) {
    trainSetSwitch(i, SWITCH_STRAIGHT);
  }
  for (int i = 1; i < 19; i++) {
    trainSetSwitch(i, SWITCH_CURVED);
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
        Putc(com2, 'a' + numWorkerLeft);
        trainSetSpeed(&msg, &numWorkerLeft);
        break;
      }
      default: {
        ASSERT(FALSE, "Not suppported train message type.");
      }
    }

  }
}

void trainSetSwitch(int sw, int state) {
  char msg[2];
  msg[0] = (char)state;
  msg[1] = (char)sw;

  Putstr(com1, msg, 2);
  //printff(com1, "Sw: %d State: %d \n", sw, state);
  switchStatus[sw] = state;
}

void trainSetSpeed(TrainMsg* origMsg, int* numWorkerLeft) {
  const int trainNum = origMsg->data1;
  const int speed = origMsg->data2;
  if (origMsg->data3 == WORKER_SIG) {
    (*numWorkerLeft)++;
    ASSERT(speed > 0, "Train worker has negative speed.");
  }

  char msg[3];
  msg[1] = (char)trainNum;
  msg[2] = 0;
  if (speed >= 0) {
    msg[0] = (char)speed;
  } else {
    msg[0] = 0;
    origMsg->data2 = train[trainNum].speed;
    origMsg->data3 = 250; // 2.5s . TODO, calculate from train speed.
    Send(worker[*numWorkerLeft], (char*)origMsg,
        sizeof(TrainMsg), (char*)1, 0);
    numWorkerLeft--;
    if (*numWorkerLeft < -1) {
      printff(com2, "Used non-existence worker id");
      while(1);
    }
  }

  Putstr(com1, msg, 3);
  train[trainNum].speed = msg[1];
}

int startTrainControllerTask() {
  return Create(2, trainController);
}
