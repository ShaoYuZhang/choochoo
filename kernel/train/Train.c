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

static int switchStatus[NUM_SWITCHES];

typedef struct Train {
  int speed;
} Train;

static int com1;
static int com2;
static Train train[NUM_TRAINS];
static int worker[NUM_WORKER];

static void trainSetSwitch(int sw, int state) {
  char msg[2];
  msg[0] = (char)state;
  msg[1] = (char)sw;

  Putstr(com1, msg, 2);
  //printff(com1, "Sw: %d State: %d \n", sw, state);
  switchStatus[sw] = state;
}

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
      msg[0] = (char)speed;
      Putstr(com1, msg, 2);
    }
    train[trainNum].speed = speed;
  } else {
    printff(com2, "Reverse... %d \n", train[trainNum].speed);
    origMsg->data2 = (signed char)train[trainNum].speed;
    origMsg->data3 = 150; // 2.5s . TODO, calculate from train speed.
    printff(com2, "Using worker: %d \n", *numWorkerLeft);

    Reply(worker[*numWorkerLeft], (char*)origMsg, sizeof(TrainMsg));
    (*numWorkerLeft)--;

    msg[0] = 0;
    msg[1] = (char)trainNum;

    if (*numWorkerLeft < -1) {
      printff(com2, "Used non-existence worker id");
      while(1);
    }
    Putstr(com1, msg, 4);
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
    int numTick = msg.data3*2; // num of 10ms
    Delay(numTick, timeserver);
    msg.data3 = WORKER;
    printff(com2, "Worker Done.\n");
    Send(parent, (char*)&msg, sizeof(TrainMsg), (char*)&msg, sizeof(TrainMsg));
  }
}

static void trainController() {
  char com1Name[] = IOSERVERCOM1_NAME;
  char com2Name[] = IOSERVERCOM2_NAME;
  char uiName[] = UI_TASK_NAME;
  char trainName[] = TRAIN_NAME;
  com1 = WhoIs(com1Name);
  com2 = WhoIs(com2Name);
#ifndef CALIBRATION
  int ui = WhoIs(uiName);
#endif
  RegisterAs(trainName);

  for (int i = 0; i < NUM_TRAINS; i++) {
    train[i].speed = 0;
  }

  int numWorkerLeft = -1;
  for (int i = 0; i < NUM_WORKER; i++) {
    worker[i] = Create(1, trainWorker);
  }

  for (int i = 1; i < 19; i++) {
    trainSetSwitch(i, SWITCH_CURVED);
  }

  UiMsg uimsg;
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
#ifndef CALIBRATION
        uimsg.type = UPDATE_SWITCH;
        uimsg.data1 = msg.data1;
        uimsg.data2 = msg.data2;
        Send(ui, (char*)&uimsg, sizeof(UiMsg), (char*)1, 0);
#endif
        break;
      }
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
