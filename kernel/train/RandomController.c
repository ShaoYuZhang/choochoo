#include "RandomController.h"
#include <Driver.h>
#include <NameServer.h>
#include <UserInterface.h>
#include <TimeServer.h>
#include <IoHelper.h>

static int ui;
static int trackController;
static int trainController;

static void delayer_task() {
  char timeServerName[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timeServerName);
  int parent = MyParentsTid();
  for (;;) {
    // 30seconds
    Delay(30*100, timeserver);
    Send(parent, (char*)1, 0, (char*)1, 0);
  }
}

static void sendRandom(int trainNum) {
  DriverMsg driveMsg;
  driveMsg.type = SET_ROUTE;
  driveMsg.data2 = 9; // speeed
  driveMsg.trainNum = trainNum;

  TrackMsg trackMsg;
  trackMsg.type = GET_RANDOM_POSITION;
  Send(trackController,
      (char*)&trackMsg, sizeof(TrackMsg), (char*)&driveMsg.pos,
      sizeof(Position));
  driveMsg.pos.offset = 0;

  PrintDebug(ui, "Random Route %d %d %d %d %d %d %d %d %d",
      trainNum, driveMsg.data2,
      driveMsg.pos.landmark1.type, driveMsg.pos.landmark1.num1,
      driveMsg.pos.landmark1.num2, driveMsg.pos.landmark2.type,
      driveMsg.pos.landmark2.num1, driveMsg.pos.landmark2.num2,
      driveMsg.pos.offset);
  Send(trainController, (char*)&driveMsg, sizeof(DriverMsg),
      (char*)NULL, 0);
}

static void randomController() {
  char randomName[] = RANDOM_CONTROL_NAME;
  RegisterAs(randomName);

  char trainControllerName[] = TRAIN_CONTROLLER_NAME;
  trainController = WhoIs(trainControllerName);
  char uiName[] = UI_TASK_NAME;
  ui = WhoIs(uiName);
  int delayer = Create(9, delayer_task);

  char trackName[] = TRACK_NAME;
  trackController = WhoIs(trackName);

  int trainNumber[2] = {-1, -1};

  for (;;) {
    int tid = -1;
    int trainNum;
    int len = Receive(&tid, (char*)&trainNum, 4);
    Reply(tid, (char*)1, 0);

    if (len == 0) {
      // Delayer and doing random moving
      for (int i = 0; i < 2; i++){
        if (trainNumber[i] != -1) {
          sendRandom(trainNumber[i]);
        }
      }
    } else {
      if (trainNumber[1] == trainNum) {
        PrintDebug(ui, "Stop random");
        trainNumber[1] = -1;
      } else if (trainNumber[0] == trainNum) {
        PrintDebug(ui, "Stop random");
        trainNumber[0] = -1;
      } else {
        if (trainNumber[0] == -1) {
          trainNumber[0] = trainNum;
          PrintDebug(ui, "Sending random position to %d", trainNum);
          sendRandom(trainNum);
        } else if (trainNumber[1] == -1) {
          trainNumber[1] = trainNum;
          PrintDebug(ui, "Sending random position to %d", trainNum);
          sendRandom(trainNum);
        } else {
          PrintDebug(ui, "No memory left to register train %d.",
              trainNum);
        }
      }
    }
  }
}

int startRandomTrainControllerTask() {
  return Create(10, randomController);
}
