#ifndef RPS_SERVER_H_
#define RPS_SERVER_H_

#define WIN  10
#define LOSE 11
#define TIE  12

#define SIGNUP 10
#define PLAY 21
#define QUIT 32

#define SCISSOR 0
#define PAPER 1
#define ROCK 2
#define HAS_QUIT 10

#define RPS_SERVER_NAME "RPS_SERVER\0\0\0"

typedef struct PlayMessage {
  char index;
  char move;
} PlayMessage;

void startServerRPS();

#endif // RPS_SERVER_H_
