#include "Train.h"
#include "IoServer.h"
#include <ts7200.h>
#include <util.h>
#include <TimeServer.h>
#include <NameServer.h>
#include <IoHelper.h>

static int switchStatus[NUM_SWITCHES];

typedef struct Train {
  int speed;
  int reversed;
  int delay;
} Train;

static int com1;
static Train train[NUM_TRAINS];

void trainGetSwitch();

void trainController() {
  char com1Name[] = IOSERVERCOM2_NAME;
  com1 = WhoIs(com1Name);
  char trainName[] = TRAIN_NAME;
  RegisterAs(trainName);

  for (int i = 0; i < NUM_TRAINS; i++) {
    train[i].speed = 0;
    train[i].reversed = 0;
  }

  for (;;) {
    int tid = -1;
    TrainMsg msg;
    Receive(&tid, (char*)&msg, sizeof(TrainMsg));

    switch (msg.type) {
      case GET_SWITCH: {
        Reply(tid, (char*)(switchStatus + msg.data1), 8);
        break;
      }
      case SET_SWITCH: {
        Reply(tid, (char*)1, 0);
        trainSetSwitch((int)msg.data1, (int)msg.data2);
        break;
      }
      case GET_SPEED: {
        const int trainNum = msg.data1;
        Reply(tid, (char*)&(train[trainNum].speed), 8);
        break;
      }
      case SET_SPEED: {
        Reply(tid, (char*)1, 0);
        const int trainNum = msg.data1;
        const int speed = msg.data2;
        trainSetSpeed(trainNum, speed);
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

  putstr(com1, msg);
  switchStatus[sw] = state;
}

void trainSetSpeed(int trainNum, int spd) {
  if (spd >= 0) {
    char msg[3];
    msg[0] = (char)spd;
    msg[1] = (char)trainNum;
    msg[2] = 0;
    train[trainNum].speed = spd;
  } else {
    // TODO
  }
}

#if 0
void train_reverse(int train) {
  timerRemoveTask(delay[train]);
  delay[train] = 0;

  bwputc(COM1, 0);
  bwputc(COM1, train);

  // Reverse train and give some time for train to stop.
  delay[train] = timerCreateTask(
      train_speed, (void*)(0x80000000 | (train<<16) | speed[train]), 3000);
}
#endif

int startTrainControllerTask() {
  return Create(2, trainController);
}


