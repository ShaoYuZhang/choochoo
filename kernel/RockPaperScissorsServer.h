#ifndef RPS_SERVER_H_
#define RPS_SERVER_H_

#define WIN  10
#define LOSE 11
#define TIE  12

#define SIGNUP 10
#define PLAY 21
#define QUIT 32

#define SCISSOR 1
#define PAPER 2
#define ROCK 3
#define HAS_QUIT 10

typedef struct PlayMessage {
		char index;
		char move;
} PlayMessage;

void startServerRPS();

#endif // RPS_SERVER_H_
