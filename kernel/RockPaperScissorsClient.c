#include "RockPaperScissorsClient.h"
#include <RockPaperScissorsServer.h>
#include <syscall.h>
#include <NameServer.h>

// Test situations:
//    1: normal game play between 2 players randomly (Tests message passing, receive block, send block, name server)
//    2: normal game play between 8 players randomly (Tests message passing, recever queue in server)
//    3: 4 players keep playing.., 2 players are stuck waiting for others.
//        (Tests that tasks that are blocked can be rewoken up.)
//    4: Print reply message
//    5: Differing priorities between server client. Result should be the same

void startClientsRPS() {
  Create(2, client1);
  Create(2, client1);
  Create(2, client1);
  Create(2, client1);
  Create(2, client1);
  Create(2, client1);
}

void client1() {

  bwprintf(COM2, "%d MY ID\n", MyTid());

  signed char server = (signed char)WhoIs(RPS_SERVER_NAME);
  bwprintf(COM2, "%d RPS SERVER \n", (int)server);

  char index = SignUp(server);
  bwprintf(COM2, "%d Client Index \n", (int)index);

  for (int i = 0; i < 2; i++) {
    int result;
    if (MyTid()%2) {
      Pass();
      result = Play(server, index, ROCK);
    } else {
      result = Play(server, index, PAPER);
    }
    bwprintf(COM2, "%d Client Result \n", (int)result);
  }

  int result = Quit(server, index);


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
