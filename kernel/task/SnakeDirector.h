#ifndef SNAKE_DIRECTOR_H_
#define SNAKE_DIRECTOR_H_

#include <Track.h>
#include <DumbDriver.h>

#define SNAKE_DIRECTOR_NAME "SNAKEKKY\0"

#define REGISTER_BAIT  1
#define REGISTER_SNAKE 2
// TODO
#define SNAKE_CAUGHT_BAIT 3

typedef struct SnakeMessage {
  char type;
  char trainNum;
} SnakeMessage;

typedef struct GamePiece {
  signed char trainNum;
  char eaten;
  struct GamePiece* food;
  char positionKnown;
  DumbDriverInfo info;
} GamePiece;

int RegisterBait(int tid, int trainNum);

int RegisterSnake(int tid, int trainNum);

int startSnakeDirectorTask();

#endif // SNAKE_DIRECTOR_H_
