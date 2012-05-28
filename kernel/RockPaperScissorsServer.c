#include "RockPaperScissorsServer.h"
#include <syscall.h>
#include <bwio.h>
#include <NameServer.h>

#define MAX_CLIENT 128

typedef struct PlayingPair {
		char tid1;
    signed char move1;
		char tid2;
    signed char move2;
} PlayingPair;

static PlayingPair  playing_array[MAX_CLIENT];
static PlayingPair* playing_stack[MAX_CLIENT];
static int num_pair_left;

signed char waiting_client;

void replyPlayingPair(int tid1, int tid2) {
  // Initialize the pair to be playing
  PlayingPair* newPair = playing_stack[num_pair_left];
  newPair->tid1 = (signed char)tid1;
  newPair->tid2 = (signed char)tid2;

  char index[2];
  index[0] = (char)(newPair - playing_array);
  index[1] = '\0';
  // Ack the clients to start playing, reply with their index playing token.
  int reply;
  reply = Reply(tid1, index, 2);
  reply = Reply(tid2, index, 2);
}

void replyWin(PlayingPair* pair) {
  signed char result = pair->move1 - pair->move2;
  pair->move1 = -1;
  pair->move2 = -1;
  char buffer[2];
  buffer[1] = 0;
  int reply;

  bwputstr(COM2, "----------------------------------------------------\n");
  if (result == 1 || result == -2) {
    bwprintf(COM2, "%d beats %d.\n", pair->tid2, pair->tid1);
    bwgetc(COM2);
    // Player 2 wins
    buffer[0] = LOSE;
    reply = Reply(pair->tid1, buffer, 2);
    buffer[0] = WIN;
    reply = Reply(pair->tid2, buffer, 2);
  }
  else if (result == -1 || result == 2) {
    bwprintf(COM2, "%d beats %d.\n", pair->tid1, pair->tid2);
    bwgetc(COM2);
    // Player 1 wins
    buffer[0] = WIN;
    reply = Reply(pair->tid1, buffer, 2);
    buffer[0] = LOSE;
    reply = Reply(pair->tid2, buffer, 2);
  } else {
    bwprintf(COM2, "%d and %d tied.\n", pair->tid1, pair->tid2);
    bwgetc(COM2);
    // Tie
    buffer[0] = TIE;
    reply = Reply(pair->tid1, buffer, 2);
    buffer[0] = TIE;
    reply = Reply(pair->tid2, buffer, 2);
  }
}

void serverTask() {
  bwputstr(COM2, "Server says Starting RPS server\n");
  RegisterAs(RPS_SERVER_NAME);
  bwputstr(COM2, "Server says Registerd server.\n");

  int tid;
  char msg[32];
  Pass();
  Pass();

  bwputstr(COM2, "Server says Begin receiving message.\n");

  while (1) {
    int len = Receive(&tid, msg, 32);
    char type = msg[len-2];

    if (type == SIGNUP) {
      if (waiting_client == -1) {
        waiting_client = (signed char)tid;
        bwprintf(COM2, "Server says client%d does not have partner\n", (int)tid);
      }
      else {
        bwprintf(COM2, "Server says client%d is now playing with %d\n",
            (int)tid, (int)waiting_client);
        replyPlayingPair(waiting_client, tid);
        waiting_client = -1;
        num_pair_left -= 1;
      }
    }
    else if (type == PLAY) {
      PlayMessage* play = (PlayMessage*)msg;
      PlayingPair* pair = &playing_array[(int)play->index];

      if (tid == pair->tid1) {
        pair->move1 = play->move;
      } else if (tid == pair->tid2) {
        pair->move2 = play->move;
      }

      if (pair->move1 != -1 && pair->move2 != -1) {
        replyWin(pair);
      } else if (pair->move1 == HAS_QUIT || pair->move2 == HAS_QUIT) {
        msg[0] = 'Q';
        msg[1] = '\0';
        Reply(tid, msg, 2);
      }
    }
    else if (type == QUIT) {
      PlayMessage* play = (PlayMessage*)msg;
      PlayingPair* pair = &playing_array[(int)play->index];
      if (tid == pair->tid1) {
        pair->move1 = HAS_QUIT;
      } else if (tid == pair->tid2) {
        pair->move2 = HAS_QUIT;
      }

      if (pair->move2 == HAS_QUIT && pair->move1 == HAS_QUIT) {
        playing_stack[++num_pair_left] = pair;
      }

      msg[0] = 'A';
      msg[1] = '\0';
      Reply(tid, msg, 2);
    }
    else {
      bwputstr(COM2, "Unknown RPS Server type\n");
    }
  }
  Exit();
}

void startServerRPS() {
  for (int i = 0; i < MAX_CLIENT; i++) {
    playing_array[i].move1 = -1;
    playing_array[i].move2 = -1;
    playing_stack[i] = &playing_array[i];
  }
  num_pair_left = MAX_CLIENT-1;
  waiting_client = -1;

  Create(2, serverTask);
}
