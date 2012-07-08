#include "DriverController.h"
#include <IoServer.h>
#include <NameServer.h>

// An admin that pass message to train.
// Additionally create train if it does not exist.
static void trainController() {
  char trainName[] = TRAIN_CONTROLLER_NAME;
  RegisterAs(trainName);

  char uiName[] = UI_TASK_NAME;
  int ui = WhoIs(uiName);
  char com1Name[] = IOSERVERCOM1_NAME;
  int com1 = WhoIs(com1Name);

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
      for (int i = 0; i < 80; i++) {
        if (trainTid[i] != -1) {
          Send(trainTid[i], (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
        }
      }
    } else {
      if (msg.trainNum >= 35 && msg.trainNum <= 44) {
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
      }
    }
  }
}

int startDriverControllerTask() {
  return Create(5, trainController);
}
