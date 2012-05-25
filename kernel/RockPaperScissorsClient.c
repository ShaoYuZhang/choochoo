#include "RockPaperScissorsClient.h"
#include <RockPaperScissorsServer.h>
#include <syscall.h>

// Test situations:
//    1: normal game play between 2 players randomly (Tests message passing, receive block, send block, name server)
//    2: normal game play between 8 players randomly (Tests message passing, recever queue in server)
//    3: 4 players keep playing.., 2 players are stuck waiting for others.
//        (Tests that tasks that are blocked can be rewoken up.)
//    4: Print reply message

void client1() {

}

int SignUp(signed char server_tid) {
  char message[10];
  message[0] = SIGNUP;
  message[1] = '\0';

  return Send(server_tid, message, 2, message, 10);
}

int Play(signed char server_tid, char index, char move) {
  char message[sizeof(PlayMessage)+3];
  PlayMessage* m = (PlayMessage*)message;
  m->index = index;
  m->move = move;

  message[sizeof(PlayMessage)+1] = '\0';
  message[sizeof(PlayMessage)+2] = PLAY;
  message[sizeof(PlayMessage)+3] = '\0';

  return Send(server_tid, message, 2, message, 10);
}

int Quit(signed char server_tid) {
  char message[10];
  message[0] = QUIT;
  message[1] = '\0';
  return Send(server_tid, message, 2, message, 10);
  return 0;
}
