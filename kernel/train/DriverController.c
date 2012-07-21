#include "DriverController.h"
#include <IoServer.h>
#include <TimeServer.h>
#include <NameServer.h>
#include <IoHelper.h>

static int ui;
int someTrainInit(int* initTrain) {
  for(int i = 0; i < 3; i++) {
    if (initTrain[i] != -1) {
      return 1;
    }
  }
  return 0;
}

void enqueue(int* initTrain, int trainNum) {
  for (int i = 0; i < 3; i++) {
    if (initTrain[i] == -1) {
      initTrain[i] = trainNum;
      return;
    }
  }
}

void dequeue(int* initTrain, int trainNum) {
  for (int i = 0; i < 3; i++) {
    if (initTrain[i] == trainNum) {
      initTrain[i] = -1;
      return;
    }
  }
}

int getNext(int* initTrain) {
  for (int i = 0; i < 3; i++) {
    if (initTrain[i] != -1) {
      return initTrain[i];
    }
  }
  return 0;
}

// An admin that pass message to train.
// Additionally create train if it does not exist.
static void trainController() {
  char trainName[] = TRAIN_CONTROLLER_NAME;
  RegisterAs(trainName);

  char uiName[] = UI_TASK_NAME;
  ui = WhoIs(uiName);
  char com1Name[] = IOSERVERCOM1_NAME;
  int com1 = WhoIs(com1Name);

  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);

  int nth = 0;
  int trainTid[80]; // Train num -> train tid
  for (int i = 0; i < 80; i++) {
    trainTid[i] = -1;
  }

  int trainWithoutPosition[3];
  for (int i = 0; i < 3; i++) {
    trainWithoutPosition[i] = -1;
  }

  for (;;) {
    int tid = -1;
    DriverMsg msg;
    msg.trainNum = -1;
    msg.data2 = -1;
    msg.data3 = -1;
    Receive(&tid, (char*)&msg, sizeof(DriverMsg));

    if (msg.type == BROADCAST_LOST) {
      // Assume no train initing.
      Reply(tid, (char*)1, 0);

      PrintDebug(ui, "Stop all trains.. a train is lost.");
      for (int i = 0; i < 80; i++) {
        if (trainTid[i] != -1) {
          DriverMsg stopMoving;
          stopMoving.type = SET_SPEED;
          stopMoving.trainNum = (char)i;
          stopMoving.data2 = 0;
          stopMoving.data3 = 100; // junk so won't be confused with delayer

          // Tell train to stop moving
          Send(trainTid[i], (char*)&stopMoving, sizeof(DriverMsg), (char*)1, 0);

          enqueue(trainWithoutPosition, i);
        }
      }

      PrintDebug(ui, "Waiting... for trains to stop");
      Delay(timeserver, 300);

      msg.type = FIND_POSITION;
      msg.trainNum = trainWithoutPosition[0];
      PrintDebug(ui, "Initing trains... %d", msg.trainNum);

    } else if (msg.type == FIND_POSITION) {
      Reply(tid, (char*)1, 0);
      if (someTrainInit(trainWithoutPosition)) {
        enqueue(trainWithoutPosition, msg.trainNum);
        continue;
      }
      enqueue(trainWithoutPosition, msg.trainNum);
    }
    else if (msg.type == KNOW_POSITION) {
      Reply(tid, (char*)1, 0);
      dequeue(trainWithoutPosition, msg.trainNum);
      int next = getNext(trainWithoutPosition);
      PrintDebug(ui, "Found %d ", msg.trainNum);
      if (next) {
        msg.trainNum = next;
        msg.type = FIND_POSITION;
        PrintDebug(ui, "Initing %d ", next);
      } else {
        continue;
      }
    } else if (someTrainInit(trainWithoutPosition)) {
      PrintDebug(ui, "Drop msg T:%d from %d cuz a train init.",
          msg.type, tid);
      Reply(tid, (char*)1, 0);
      continue;
    }

    if (msg.trainNum == 255) {
      // Broadcast, can't receive replies
      Reply(tid, (char*)1, 0);
      int count = 0;
      for (int i = 0; i < 80; i++) {
        if (trainTid[i] != -1) {
          msg.data3 = count;
          count++;
          Send(trainTid[i], (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
        }
      }
    } else if (msg.type == MERGE) {
      // TODO two trains only currently
      MultiTrainInitMsg init;
      init.nth = nth;
      init.trainNum[0] = (int)msg.trainNum;
      init.trainNum[1] = (int)msg.data2;
      init.numTrain = 2;
      init.com1 = com1;
      // Create train task
      // TODO doesn't generalize if train is already inited
      trainTid[(int)msg.trainNum] = Create(4, multitrain_driver);
      Send(trainTid[(int)msg.trainNum],
          (char*)&init, sizeof(MultiTrainInitMsg), (char*)1, 0);
      nth+=2; //TODO, generalize
      Reply(tid, (char*)1, 0);
    } else {
      if (msg.trainNum >= 35 && msg.trainNum <= 45) {
        if (trainTid[(int)msg.trainNum] == -1) {
          DriverInitMsg init;
          init.nth = nth;
          init.trainNum = (int)msg.trainNum;
          init.com1 = com1;
          // Create train task
          trainTid[(int)msg.trainNum] = Create(4, driver);
          Send(trainTid[(int)msg.trainNum],
              (char*)&init, sizeof(DriverInitMsg), (char*)1, 0);
          nth++;
        }

        msg.replyTid = (char)tid;
        // Pass the message on.
        Send(trainTid[(int)msg.trainNum], (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
      }
      else {
        PrintDebug(ui, "Bad train num %d", msg.trainNum);
        Reply(tid, (char*)1, 0);
      }
    }
  }
}

void FinishPositionFinding(int trainNum, int controllerTid) {
  if (trainNum == 255) {
    PrintDebug(ui, "Cannot broadcast finish position.");
  }
  DriverMsg msg;
  msg.type = KNOW_POSITION;
  msg.trainNum = (char)trainNum;
  Send(controllerTid, (char*)&msg, sizeof(DriverMsg), (char *)NULL, 0);
}

void BroadcastLost(int controllerTid) {
  DriverMsg msg;
  msg.type = BROADCAST_LOST;
  msg.trainNum = (char)255; // Broadcast
  msg.data2 = 0;            // Speed zero.
  Send(controllerTid, (char*)&msg, sizeof(DriverMsg), (char *)NULL, 0);
}

int startDriverControllerTask() {
  return Create(5, trainController);
}
