#include "SnakeDirector.h"
#include <Driver.h>
#include <NameServer.h>
#include <CalibrationData.h>
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
  msg.type = GET_POSITION;
  if (snake->trainNum != -1) {
    msg.trainNum = (char)snake->trainNum;
    int l = Send(trainController, (char*)&msg, sizeof(msg),
        (char*)&snake->info, sizeof(DumbDriverInfo));
    snake->positionKnown = (l == sizeof(DumbDriverInfo));
  }

  for (int i = 0; i < 4; i++) {
    if (baits[i].trainNum != -1 && baits[i].eaten == 0) {
      msg.trainNum = (char)baits[i].trainNum;
      int l = Send(trainController, (char*)&msg, sizeof(msg),
          (char*)&baits[i].info, sizeof(DumbDriverInfo));
      baits[i].positionKnown = (l == sizeof(DumbDriverInfo));
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
    if (baits[i].trainNum != -1 &&
        baits[i].positionKnown &&
        baits[i].eaten == 0) {
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
    Position dest = snake->food->info.pos;

    // If previous sensor is end,
    // reverse the train to get a routable previous sensor.
    if (dest.landmark1.type == LANDMARK_END) {
      ReverseTrain(trainController, snake->food->trainNum);
      Delay(10, timeserver); // HACK. Wait for train to reverse.
      try_update_position(snake, baits);
      dest = snake->food->info.pos;
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

    Delay(170, timeserver);

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
  int CC = 0;

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
        QueryDistance(trackController,
            &snake.info.pos, &snake.food->info.pos, &distance);

        if (distance >= 0) {
          // Get more accurate distance based on knowledge of train direciton.
          // Bait first, snake chases from behind.
          distance -= snake.food->info.lenBackOfPickup;
          distance -= snake.info.lenFrontOfPickup;
          distance -= PICKUP_LEN;
          if (distance < 0) {
            distance = 0;
          }
        }

        if (distance < 150 && distance >= 0) {
          PrintDebug(ui, "Close enough %dmm.. Snake ate bait", distance);

          // Stop the trains.
          SetSpeedTrain(trainController, snake.trainNum, 0);
          SetSpeedTrain(trainController, snake.food->trainNum, 0);

          // Snake came from behind bait.. bait takes on as head.
          DoTrainMerge(trainController, snake.food->trainNum, snake.trainNum);
          PrintDebug(ui, "%d is head, %d is tail",
              snake.food->trainNum, snake.trainNum);
          Delay(100, timeserver);

          // The bait becomes the snake head.
          snake.trainNum = snake.food->trainNum;
          snake.eaten = 0;
          snake.positionKnown = 1;
          snake.info = snake.food->info;

          // Clear bait info
          snake.food->trainNum = -1;
          snake.food->eaten = 0;
          snake.food->positionKnown = 0;

          snake.food = (GamePiece*)NULL;
          CC = 0;
        } else if (distance < 500 && distance >= 0 && snake.info.velocity == 0) {
          // Nudge closer..
          SetSpeedTrain(trainController, snake.trainNum, 2);
        } else if (distance < 500) {
          if ((++CC & 7) == 0) PrintDebug(ui, "Almost there %dmm", distance);
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

  if (trainNum == 45 ||
      trainNum == 39 ||
      trainNum == 37 ||
      trainNum == 35 ||
      trainNum == 48 ||
      trainNum == 44 ||
      trainNum == 43) {
    char ret = -1;
    int len = Send(tid, (char*)&msg, sizeof(SnakeMessage), &ret, 1);
    return len != 1 || ret != 0;
  }
  return 1;
}

int RegisterSnake(int tid, int trainNum) {
  SnakeMessage msg;
  msg.type = REGISTER_SNAKE;
  msg.trainNum = (char)trainNum;

  if (trainNum == 45 ||
      trainNum == 39 ||
      trainNum == 37 ||
      trainNum == 35 ||
      trainNum == 48 ||
      trainNum == 44 ||
      trainNum == 43) {
    char ret = -1;
    int len = Send(tid, (char*)&msg, sizeof(SnakeMessage), &ret, 1);
    return len != 1 || ret != 0;
  }
  return 1;
}

int startSnakeDirectorTask(){
  return Create(10, snakeDirector);
}
