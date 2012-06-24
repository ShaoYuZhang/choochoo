#include "Train.h"
#include <IoServer.h>
#include <ts7200.h>
#include <util.h>
#include <TimeServer.h>
#include <NameServer.h>
#include <IoHelper.h>
#include <syscall.h>
#include <UserInterface.h>

#define NUM_WORKER 4

typedef struct Train {
  int speed;
} Train;

static int com1;
static int com2;
static Train train[NUM_TRAINS];
static int worker[NUM_WORKER];


static void trainSetSpeed(TrainMsg* origMsg, int* numWorkerLeft) {
  const int trainNum = origMsg->data1;
  const int speed = origMsg->data2;

  char msg[4];
  msg[1] = (char)trainNum;
  if (speed >= 0) {
    if (origMsg->data3 == WORKER) {
      printff(com2, "Reversing speed. cuz its worker %d\n", speed);
      msg[0] = 0xf;
      msg[1] = (char)trainNum;
      msg[2] = (char)speed;
      msg[3] = (char)trainNum;
      Putstr(com1, msg, 4);
    } else {
      printff(com2, "Set speed. %d %d\n", speed, trainNum);
      msg[0] = (char)speed;
      Putstr(com1, msg, 2);
    }
    train[trainNum].speed = speed;
  } else {
    printff(com2, "Reverse... %d \n", train[trainNum].speed);
    origMsg->data2 = (signed char)train[trainNum].speed;
    origMsg->data3 = 150; // 3s . TODO, calculate from train speed.
    printff(com2, "Using worker: %d \n", *numWorkerLeft);

    Reply(worker[*numWorkerLeft], (char*)origMsg, sizeof(TrainMsg));
    (*numWorkerLeft)--;

    msg[0] = 0;
    msg[1] = (char)trainNum;

    if (*numWorkerLeft < -1) {
      printff(com2, "Used non-existence worker id");
      while(1);
    }
    Putstr(com1, msg, 2);
    train[trainNum].speed = 0;
  }
}

// Responsider for delag msg server about positive train speed
static void trainWorker() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  TrainMsg msg;
  msg.type = WORKER;
  Send(parent, (char*)&msg, sizeof(TrainMsg), (char*)&msg, sizeof(TrainMsg));
  for (;;) {
    int numTick = msg.data3; // num of 10ms
    numTick *= 2;
    Delay(numTick, timeserver);
    msg.data3 = WORKER;
    //printff(com2, "Worker Done. %d\n", numTick);
    Send(parent, (char*)&msg, sizeof(TrainMsg), (char*)&msg, sizeof(TrainMsg));
  }
}

static void trainController() {
  char com1Name[] = IOSERVERCOM1_NAME;
  char com2Name[] = IOSERVERCOM2_NAME;
  char trainName[] = TRAIN_NAME;
  com1 = WhoIs(com1Name);
  com2 = WhoIs(com2Name);

  RegisterAs(trainName);

  for (int i = 0; i < NUM_TRAINS; i++) {
    train[i].speed = 0;
  }

  int numWorkerLeft = -1;
  for (int i = 0; i < NUM_WORKER; i++) {
    worker[i] = Create(1, trainWorker);
  }

  for (;;) {
    int tid = -1;
    TrainMsg msg;
    msg.data1 = -1;
    msg.data2 = -1;
    msg.data3 = -1;
    Receive(&tid, (char*)&msg, sizeof(TrainMsg));

    switch (msg.type) {
      case GET_SPEED: {
        const int trainNum = msg.data1;
        Reply(tid, (char*)&(train[trainNum].speed), 4);
        break;
      }
      case SET_SPEED: {
        trainSetSpeed(&msg, &numWorkerLeft);
        if (msg.data3 != WORKER) {
          Reply(tid, (char*)1, 0);
          break;
        }
        // else fall through
      }
      case WORKER: {
        worker[++numWorkerLeft] = tid;
        break;
      }
      default: {
        ASSERT(FALSE, "Not suppported train message type.");
      }
    }
  }
}

int startTrainControllerTask() {
  return Create(2, trainController);
}
