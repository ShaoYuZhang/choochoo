#include "RockPaperScissorsClient.h"
#include <RockPaperScissorsServer.h>
#include <syscall.h>
#include <NameServer.h>

// Test situations:
//    1: normal game play between 2 players randomly (Tests message passing, receive block, send block, name server)
//    2: normal game play between 8 players randomly (Tests message passing, recever queue in server)
//    3: 4 players keep playing.., 2 players are stuck waiting for others.
//        (Tests that tasks that are blocked can be rewoken up.)

void startClientsRPS() {
  Create(2, client1);
  Create(2, client1);
}

void printResult(int result, int tid) {
  if (result == WIN) {
    bwprintf(COM2, "-----Client%d heard from server that it WON\n", tid);
  } else if (result == LOSE) {
    bwprintf(COM2, "-----Client%d heard from server that it LOST\n", tid);
  } else if (result == TIE) {
    bwprintf(COM2, "-----Client%d heard from server that it TIED\n", tid);
  }
}

void client1() {
  int result;
  bwprintf(COM2, "-----Client begins:\n");
  int mytid = MyTid();
  bwprintf(COM2, "-----Client got tid:%d\n", mytid);
  signed char server = (signed char)WhoIs(RPS_SERVER_NAME);
  bwprintf(COM2, "-----Client%d got server with tid:%d\n", mytid, (int)server);

  char index = SignUp(server);
  bwprintf(COM2, "-----Client%d signed up with server\n", mytid, (int)index);

  int move = mytid;
  for (int i = 0; i < 3; i++) {
    move += mytid;
    if (move%3 == 0) {
      bwprintf(COM2, "-----Client%d is playing SCISSORS\n", mytid);
    } else if (move%3 == 1) {
      bwprintf(COM2, "-----Client%d is playing PAPER\n", mytid);
    } else {
      bwprintf(COM2, "-----Client%d is playing ROCK\n", mytid);
    }
    result = Play(server, index, move%3);
    printResult(result, mytid);
  }

  result = Quit(server, index);

  Exit();
}

char SignUp(signed char server_tid) {
  char message[4];
  message[0] = SIGNUP;
  message[1] = '\0';

  Send(server_tid, message, 2, message, 2);

  return message[0];
}

char Play(signed char server_tid, char index, char move) {
  const int size = sizeof(PlayMessage)+3;
  char message[size];

  PlayMessage* m = (PlayMessage*)message;
  m->index = index;
  m->move = move;

  message[sizeof(PlayMessage)  ] = '\0';
  message[sizeof(PlayMessage)+1] = PLAY;
  message[sizeof(PlayMessage)+2] = '\0';
  Send(server_tid, message, size, message, size);
  return message[0];
}

char Quit(signed char server_tid, char index) {
  const int size = sizeof(PlayMessage)+3;
  char message[size];

  PlayMessage* m = (PlayMessage*)message;
  m->index = index;

  message[sizeof(PlayMessage)  ] = '\0';
  message[sizeof(PlayMessage)+1] = QUIT;
  message[sizeof(PlayMessage)+2] = '\0';
  Send(server_tid, message, size, message, size);

  return message[0];
}
