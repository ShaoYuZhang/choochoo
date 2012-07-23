#include "SnakeDirector.h"
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

static void try_notify_snake(GamePiece* snake, GamePiece* baits) {
  if (snake->eating) return;

  // Try to find a bait for snake.
  GamePiece* bait = (GamePiece*)NULL;
  for (int i = 0; i < 4; i++) {
    if (baits[i].trainNum != -1 && baits[i].positionKnown) {
      bait = &(baits[i]);
      break;
    }
  }

  // If we got a bait for snake.
  if (bait != (GamePiece*)NULL) {
    DriverMsg driveMsg;
    driveMsg.type = SET_ROUTE;
    driveMsg.data2 = 9; // speed
    driveMsg.trainNum = bait->trainNum;
    driveMsg.pos = bait->pos;

    PrintDebug(ui, "Snake going for bait %d", bait->trainNum);
    Send(trainController, (char*)&driveMsg, sizeof(DriverMsg),
        (char*)NULL, 0);
    snake->eating = 1;
  }
}

static void snakeDirector() {
  char directorName[] = SNAKE_DIRECTOR_NAME;
  RegisterAs(directorName);

  char trainControllerName[] = TRAIN_CONTROLLER_NAME;
  trainController = WhoIs(trainControllerName);
  char uiName[] = UI_TASK_NAME;
  ui = WhoIs(uiName);
  int delayer = Create(9, delayer_task);

  // Initialization
  GamePiece baits[4];
  for(int i = 0; i < 4; i++) {
    baits[i].trainNum = -1;
    baits[i].eaten = 0;
    baits[i].positionKnown = 0;
  }
  GamePiece snake;
  snake.trainNum = -1;
  snake.eaten = 0;
  snake.eating = 0;
  snake.positionKnown = 0;

  // Handling events.
  for (;;) {
    int tid = -1;
    SnakeMessage msg;
    int len = Receive(&tid, (char*)&msg, sizeof(SnakeMessage));

    if (len == 0) {
      // Nagger..
      // TODO if snake is not eating..
      // Asks each bait about its position if it hasn'
      try_notify_snake(&snake, baits);
      Reply(tid, (char*)1, 0);
    } else if (msg.type == REGISTER_BAIT) {
      int trainNum = msg.trainNum;
      PrintDebug(ui, "Reigster %d as bait", trainNum);

      char fail = 1;
      // Can register if there is empty slot. ASSUME no duplicate bait.
      for (int i = 0; i < 4; i++) {
        if (baits[i].trainNum == -1) {
          baits[i].trainNum = trainNum;
          fail = 0;
          break;
        }
      }
      Reply(tid, (char*)&fail, 1);
      try_notify_snake(&snake, baits);
    } else if (msg.type == REGISTER_SNAKE) {
      int trainNum = msg.trainNum;
      PrintDebug(ui, "Reigster %d as snake", trainNum);
      char fail = 1;
      if ((snake.trainNum == -1)) {
        snake.trainNum = trainNum;
        fail = 0;
      }
      Reply(tid, (char*)&fail, 1);
      try_notify_snake(&snake, baits);
    } else {
      PrintDebug(ui, "Invalid type %d to snake director from tid", tid);
    }
  }
}

int RegisterBait(int tid, int trainNum) {
  SnakeMessage msg;
  msg.type = REGISTER_BAIT;
  msg.trainNum = (char)trainNum;

  char ret = -1;
  int len = Send(tid, (char*)&msg, sizeof(SnakeMessage), &ret, 1);
  return len != 1 || ret != 0;
}

int RegisterSnake(int tid, int trainNum) {
  SnakeMessage msg;
  msg.type = REGISTER_SNAKE;
  msg.trainNum = (char)trainNum;

  char ret = -1;
  int len = Send(tid, (char*)&msg, sizeof(SnakeMessage), &ret, 1);
  return len != 1 || ret != 0;
}

int startSnakeDirectorTask(){
  return Create(10, snakeDirector);
}