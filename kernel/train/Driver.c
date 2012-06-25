#include "Driver.h"
#include <IoServer.h>
#include <ts7200.h>
#include <util.h>
#include <TimeServer.h>
#include <NameServer.h>
#include <IoHelper.h>
#include <syscall.h>
#include <UserInterface.h>

typedef struct Driver {
  int speed;
  int worker;
  int ui;
  int nth; // the nth driver to be initialized. For UI display
} Driver;

static int com1;
static int com2;

static void trainSetSpeed(DriverMsg* origMsg, Driver* me) {
  const int trainNum = origMsg->trainNum;
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
    me->speed = speed;
  } else {
    printff(com2, "Reverse... %d \n", me->speed);
    origMsg->data2 = (signed char)me->speed;
    // TODO, calculate from train speed.
    origMsg->data3 = 150; // YES IT IS 3s
    printff(com2, "Using worker: %d \n", me->worker);

    Reply(me->worker, (char*)origMsg, sizeof(DriverMsg));

    msg[0] = 0;
    msg[1] = (char)trainNum;

    Putstr(com1, msg, 2);
    me->speed = 0;
  }
}

// Responsider for delag msg server about positive train speed
static void trainWorker() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = WORKER;
  for (;;) {
    Send(parent, (char*)&msg, sizeof(DriverMsg), (char*)&msg, sizeof(DriverMsg));
    int numTick = msg.data3; // num of 10ms
    numTick *= 2;
    Delay(numTick, timeserver);
    msg.data3 = WORKER;
    //printff(com2, "Worker Done. %d %d\n", numTick, parent);
  }
}

static void trainUiUpdate() {
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);
  int parent = MyParentsTid();

  DriverMsg msg;
  msg.type = UPDATE_UI;
  for (;;) {
    Delay(500, timeserver); // .5 seconds
    Send(parent, (char*)&msg, 1, (char*)1, 0);
  }
}

static void driver() {
  Driver me;
  me.speed = 0;
  me.worker = Create(1, trainWorker);
  me.ui     = Create(3, trainUiUpdate);
  int controller;
  Receive(&controller, (char*)&me.nth, 4);

  for (;;) {
    int tid = -1;
    DriverMsg msg;
    msg.trainNum = -1;
    msg.data2 = -1;
    msg.data3 = -1;
    msg.replyTid = -1;
    Receive(&tid, (char*)&msg, sizeof(DriverMsg));
    if (tid != me.worker) {
      Reply(tid, (char*)1, 0);
    }
    const int replyTid = msg.replyTid;

    switch (msg.type) {
      case GET_SPEED: {
        Reply(replyTid, (char*)&me.speed, 4);
        break;
      }
      case SET_SPEED: {
        trainSetSpeed(&msg, &me);
        if (msg.data3 != WORKER) {
          //printff(com2, "Replied to %d\n", replyTid);
          Reply(replyTid, (char*)1, 0);
          break;
        }
        // else fall through
        //printff(com2, "Worker setted speed%d\n", me.speed);
      }
      case WORKER: {
        // Worker reporting back.
        break;
      }
      case UPDATE_UI: {
        // Make ui report
        break;
      }
      default: {
        ASSERT(FALSE, "Not suppported train message type.");
      }
    }
  }
}

// Basically a courier to pass message to train.
// Additionally create train if it does not exist.
static void trainController() {
  char trainName[] = TRAIN_CONTROLLER_NAME;
  RegisterAs(trainName);

  char com1Name[] = IOSERVERCOM1_NAME;
  char com2Name[] = IOSERVERCOM2_NAME;
  com1 = WhoIs(com1Name);
  com2 = WhoIs(com2Name);

  int numTrain = 0;
  int trainTid[80]; // Train num -> train tid
  for (int i = 0; i < 80; i++) {
    trainTid[i] = -1;
  }

  for (;;) {
    int tid = -1;
    DriverMsg msg;
    msg.trainNum = -1;
    msg.data2 = -1;
    msg.data3 = -1;
    Receive(&tid, (char*)&msg, sizeof(DriverMsg));

    if (trainTid[(int)msg.trainNum] == -1) {
      // Create train task
      trainTid[(int)msg.trainNum] = Create(4, driver);
      Send(trainTid[(int)msg.trainNum], (char*)&numTrain, 4, (char*)1, 0);
      numTrain++;
    }

    msg.replyTid = (char)tid;
    // Pass the message on.
    Send(trainTid[(int)msg.trainNum], (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
  }
}

int startDriverControllerTask() {
  return Create(5, trainController);
}
