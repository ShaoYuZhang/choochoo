#include "DriverController.h"
#include <IoServer.h>
#include <NameServer.h>
#include <IoHelper.h>
#include <MultiTrainDriver.h>

static int ui;

// An admin that pass message to train.
// Additionally create train if it does not exist.
static void trainController() {
  char trainName[] = TRAIN_CONTROLLER_NAME;
  RegisterAs(trainName);

  char uiName[] = UI_TASK_NAME;
  ui = WhoIs(uiName);

  int nth = 0;
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
    } else {
      if (msg.trainNum >= 35 && msg.trainNum <= 48) {
        if (trainTid[(int)msg.trainNum] == -1) {
          trainTid[(int)msg.trainNum] =
            createMultitrainDriver(nth, msg.trainNum);
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


void DoBaitPositionFinding(int controllerTid, int trainNum) {
  DriverMsg msg;
  msg.type = FIND_POSITION;
  msg.data2 = NO_RESERVE;
  msg.trainNum = trainNum;
  Send(controllerTid, (char*)&msg, sizeof(DriverMsg), (char *)NULL, 0);
}

void DoPositionFinding(int controllerTid, int trainNum) {
  if (trainNum == 45 ||
      trainNum == 39 ||
      trainNum == 37 ||
      trainNum == 35 ||
      trainNum == 48 ||
      trainNum == 44 ||
      trainNum == 43) {
    DriverMsg msg;
    msg.type = FIND_POSITION;
    msg.data2 = RESERVE;
    msg.trainNum = trainNum;
    Send(controllerTid, (char*)&msg, sizeof(DriverMsg), (char *)NULL, 0);
  }
}

void DoTrainMerge(int controllerTid, int headTrainNum, int tailTrainNum) {
  int headTid = WhoIsMulti(headTrainNum);
  int tailTid = WhoIsMulti(tailTrainNum);
  if (headTid == 0) {
    PrintDebug(ui, "Train %d was not registered.", headTrainNum);
  } else if (tailTid == 0) {
    PrintDebug(ui, "Train %d was not registered.", tailTrainNum);
  }
  PrintDebug(ui, "Train %d was registered.", headTid);
  PrintDebug(ui, "Train %d was registered.", tailTid);

  // Tell tail train to enter courier mode between dumb train and multi-train
  DriverMsg msg;
  msg.type = MERGE_TAIL;
  msg.trainNum = tailTrainNum;
  msg.data2 = headTid;
  Send(controllerTid, (char*)&msg, sizeof(DriverMsg), (char *)NULL, 0);

  // Notify head train about new tail.
  msg.type = MERGE_HEAD;
  msg.trainNum = headTrainNum;
  msg.data2 = tailTid;
  Send(controllerTid, (char*)&msg, sizeof(DriverMsg), (char *)NULL, 0);
}

void BroadcastLost(int controllerTid) {
  PrintDebug(ui, "Broadcast lost DISABLED");
  /*
  DriverMsg msg;
  msg.type = BROADCAST_LOST;
  msg.trainNum = (char)255; // Broadcast
  msg.data2 = 0;            // Speed zero.
  Send(controllerTid, (char*)&msg, sizeof(DriverMsg), (char *)NULL, 0);
  */
}

void ReverseTrain(int controllerTid, int trainNum){
  DriverMsg msg;
  msg.type = SET_SPEED;
  msg.trainNum = trainNum;
  msg.data2 = -1;
  Send(controllerTid, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
}

void SetSpeedTrain(int controllerTid, int trainNum, int spd){
  DriverMsg msg;
  msg.type = SET_SPEED;
  msg.trainNum = trainNum;
  msg.data2 = spd;
  Send(controllerTid, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
}

void SetFollowingDistance(int controllerTid, int trainNum, int minDist, int maxDist) {
  DriverMsg msg;
  msg.type = SET_FOLLOWING_DISTANCE;
  msg.trainNum = trainNum;
  msg.data2 = minDist;
  msg.data3 = maxDist;
  Send(controllerTid, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
}

int startDriverControllerTask() {
  return Create(5, trainController);
}
