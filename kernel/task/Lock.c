#include "Lock.h"
#include <syscall.h>
#include <TimeServer.h>

#define LOCK 32
#define UNLOCK 16

static int id;
static char NO;
static char YES;

static void lock_task() {
  int locked = 0; // Start unlocked.
  for (;;) {
    int tid;
    char request;
    Receive(&tid, &request, 1);
    if (request == LOCK){
      if (locked) {
        Reply(tid, &NO, 1);
      } else {
        locked = 1;
        Reply(tid, &YES, 1);
      }
    } else {
      locked = 0;
      Reply(tid, &YES, 1);
    }
  }
}

int startLockServiceTask() {
  NO = 1;
  YES = 0;
  id = Create(4, lock_task);
  return id;
}

void lock(int timeserver) {
  char request = LOCK;
  char reply = NO;
  do {
    Send(id, &request, 1, (char*)&reply, 1);
  } while (
      reply == NO &&
      Delay(80+GET_TIMER4()%20, timeserver)); // backoff for ~1 sec.
}

void unlock() {
  char request = UNLOCK;
  Send(id, &request, 1, (char*)1, 0);
}
