#include "SnakeDirector.h"
#include <Driver.h>
#include <NameServer.h>
#include <UserInterface.h>
#include <TimeServer.h>
#include <MultiTrainDriver.h>
#include <DriverController.h>
#include <IoHelper.h>
#include <Track.h>

static int ui;
static int trackController;
static int trainController;
static int timeserver;

static void printLandmark(int ui, TrackLandmark* l) {
  if (l->type == LANDMARK_SENSOR) {
    PrintDebug(ui, "Landmark Sn  %c%d",
        'A' +l->num1, l->num2);
  } else if (l->type == LANDMARK_END) {
    PrintDebug(ui, "Landmark %s %d",
        l->num1 == EN ? "EN" : "EX",
        l->num2);
  } else if (l->type == LANDMARK_FAKE) {
    PrintDebug(ui, "Landmark Fake %d %d",
        l->num1, l->num2);
  } else if (l->type == LANDMARK_SWITCH) {
    PrintDebug(ui, "Landmark Switch Num:%d Type:%s",
        l->num2, l->num1 == MR ? "MR" : "BR");
  } else if (l->type == LANDMARK_BAD) {
    PrintDebug(ui, "Landmark type: bad.");
  } else {
    PrintDebug(ui, "Unknown Landmark type: %d", l->type);
  }
}

static void delayer_task() {
  int parent = MyParentsTid();
  for (;;) {
    Delay(25, timeserver); // 4 times a second
    Send(parent, (char*)1, 0, (char*)1, 0);
  }
}

static void try_update_position(GamePiece* snake, GamePiece* baits) {
  DriverMsg msg;
  msg.trainNum = (char)snake->trainNum;
  msg.type = GET_POSITION;
  int l = Send(trainController, (char*)&msg, sizeof(msg),
      (char*)&snake->pos, sizeof(snake->pos));
  snake->positionKnown = (l == sizeof(snake->pos));

  for (int i = 0; i < 4; i++) {
    if (baits[i].trainNum != -1 && baits[i].eaten == 0) {
      msg.trainNum = (char)baits[i].trainNum;
      int l = Send(trainController, (char*)&msg, sizeof(msg),
          (char*)&baits[i].pos, sizeof(baits[i].pos));
      baits[i].positionKnown = (l == sizeof(baits[i].pos));
    }
  }
}

static void ReverseSensor(TrackLandmark* sensor) {
  int action = sensor->num2%2 == 1 ? 1 : -1;
  sensor->num2 += action;
}

static void QueryNextSensor(char box, char val, TrackNextSensorMsg* trackMsg) {
  TrackMsg qMsg;
  qMsg.type = QUERY_NEXT_SENSOR_FROM_SENSOR;
  qMsg.landmark1.type = LANDMARK_SENSOR;
  qMsg.landmark1.num1 = box;
  qMsg.landmark1.num2 = val;
  Send(trackController, (char*)&qMsg, sizeof(TrackMsg),
      (char*)trackMsg, sizeof(TrackNextSensorMsg));
}

static void try_notify_snake(GamePiece* snake, GamePiece* baits) {
  if (snake->food != (GamePiece*)NULL || !snake->positionKnown) return;

  // Try to find a bait for snake.
  for (int i = 0; i < 4; i++) {
    if (baits[i].trainNum != -1 && baits[i].positionKnown) {
      snake->food = &(baits[i]);
      break;
    }
  }

  // If we got a bait for snake.
  if (snake->food != (GamePiece*)NULL) {
    PrintDebug(ui, "Notifying snake about bait %d", snake->food->trainNum);

    // Release reservation so snake can go there.
    clearReservation(trackController, snake->food->trainNum);

    // Find a route to bait without colliding into the bait.
    Position dest = snake->food->pos;

    // If previous sensor is end,
    // reverse the train to get a routable previous sensor.
    if (dest.landmark1.type == LANDMARK_END) {

      ReverseTrain(trainController, snake->food->trainNum);
      Delay(timeserver, 10); // HACK. Wait for train to reverse.
      try_update_position(snake, baits);
      dest = snake->food->pos;
      if (dest.landmark1.type == LANDMARK_END) {
        PrintDebug(ui, "Previous still END after reverse");
      }
    }
    // Now previous sensor is not LANDMARK_END;

    PrintDebug(ui, "Tentative Dest. offset:%d", dest.offset);
    printLandmark(ui, &dest.landmark1);
    printLandmark(ui, &dest.landmark2);

    if (dest.offset < 300) {
      // Move a position sensor.
      dest.landmark2 = dest.landmark1;

      // Snake cannot stop after previous sensor (300mm).

      TrackNextSensorMsg nextSensor;
      ReverseSensor(&dest.landmark1);
      //PrintDebug(ui, "Reversed landmark1");
      //printLandmark(ui, &dest.landmark1);

      QueryNextSensor(dest.landmark1.num1, dest.landmark1.num2, &nextSensor);
      int stop = nextSensor.predictions[0].dist - (300 - dest.offset);
      dest.offset = stop;
      //                           Bait===>>>>
      // -----S-------------------[--s-------]===>>------s--
      //                             |_offset|
      //      |_____dist_____________|
      //      |________stop______|
      //         .....[ snake    ]==>>>
      dest.landmark1 = nextSensor.predictions[0].sensor;
      ReverseSensor(&dest.landmark1);
      //PrintDebug(ui, "Next sensor after reverse numpred:%d", nextSensor.numPred);
      //printLandmark(ui, &dest.landmark1);
    } else {
      dest.offset = dest.offset - 300;
    }


    DriverMsg driveMsg;
    driveMsg.type = SET_ROUTE;
    driveMsg.data2 = 9; // speed
    driveMsg.data3 = ONE_PATH_DEST; // Must include landmark1's edge.
    driveMsg.trainNum = snake->trainNum;
    driveMsg.pos = dest;

    PrintDebug(ui, "Snake going for bait %d offset:%d",
        snake->food->trainNum, dest.offset);
    printLandmark(ui, &dest.landmark1);
    printLandmark(ui, &dest.landmark2);
    Send(trainController, (char*)&driveMsg, sizeof(DriverMsg),
        (char*)NULL, 0);
  }
}

static void snakeDirector() {
  char directorName[] = SNAKE_DIRECTOR_NAME;
  RegisterAs(directorName);

  char trainControllerName[] = TRAIN_CONTROLLER_NAME;
  trainController = WhoIs(trainControllerName);
  char uiName[] = UI_TASK_NAME;
  ui = WhoIs(uiName);
  char trackName[] = TRACK_NAME;
  trackController = WhoIs(trackName);
  char timeServerName[] = TIMESERVER_NAME;
  timeserver = WhoIs(timeServerName);
  int delayer = Create(9, delayer_task);

  // Initialization
  GamePiece baits[4];
  for (int i = 0; i < 4; i++) {
    baits[i].trainNum = -1;
    baits[i].eaten = 0;
    baits[i].positionKnown = 0;
  }
  GamePiece snake;
  snake.trainNum = -1;
  snake.eaten = 0;
  snake.food = (GamePiece*)NULL;
  snake.positionKnown = 0;

  // Handling events.
  for (;;) {
    int tid = -1;
    SnakeMessage msg;
    int len = Receive(&tid, (char*)&msg, sizeof(SnakeMessage));

    //=================================================================
    if (len == 0) {
      try_update_position(&snake, baits);
      try_notify_snake(&snake, baits);
      if (snake.food != (GamePiece*)NULL) {
        int distance = 2000;
        QueryDistance(trackController, &snake.pos, &snake.food->pos, &distance);
        if (distance < 350) {
          PrintDebug(ui, "Close enough %dmm.. Snake ate bait", distance);
          PrintDebug(ui, "Snake Position. offset:%d", snake.pos.offset);
          printLandmark(ui, &snake.pos.landmark1);
          printLandmark(ui, &snake.pos.landmark2);
          PrintDebug(ui, "Food position. offset:%d", snake.food->pos.offset);
          printLandmark(ui, &(snake.food->pos.landmark1));
          printLandmark(ui, &(snake.food->pos.landmark2));

          // Snake came from behind bait.. bait takes on as head.
          //DoTrainMerge(trainController, snake.food->trainNum, snake.trainNum);

          snake.food->eaten = 1;
          snake.food = (GamePiece*)NULL;
        }
      }
      Reply(tid, (char*)1, 0);
    }  //=================================================================
    else if (msg.type == REGISTER_BAIT) {
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
    }  //=================================================================
    else if (msg.type == REGISTER_SNAKE) {
      int trainNum = msg.trainNum;
      PrintDebug(ui, "Reigster %d as snake", trainNum);
      char fail = 1;
      if (snake.trainNum == -1) {
        snake.trainNum = trainNum;
        fail = 0;
      } else {
        PrintDebug(ui, "Cannot register two snakes .. yet", trainNum);
      }
      Reply(tid, (char*)&fail, 1);
    }  //=================================================================
    else {
      PrintDebug(ui, "Invalid type %d to snake director from tid", tid);
      Reply(tid, (char*)1, 0);
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
