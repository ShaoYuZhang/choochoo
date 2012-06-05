#include "NameServer.h"
#include <memory.h>
#include <string.h>

#define NUM_TASK_NAMESERVER 20
#define MSG_LEN 31

typedef struct Task {
		signed char tid;
    char name[MSG_LEN];
} Task;

static void nameserver_task() {
  Task tasks[NUM_TASK_NAMESERVER];
  char msg[MSG_LEN];
  int emptyTaskName = 0;

  while (1) {
    int tid = -1;
    int len = Receive(&tid, msg, MSG_LEN);
    char type = msg[len-1];
    //bwprintf(COM2, "Receive: %d\n", len);
    //bwprintf(COM2, "Type: %d\n", (int)type);
    //bwprintf(COM2, "Tid: %d\n", tid);
    //bwprintf(COM2, "len: %d\n", len);

    if (type == WHO_IS) {
      int found = -1;
      for (int i = 0; i < emptyTaskName; i++) {
        if (equal(msg, tasks[i].name, len-2)) {
          found = i; break;
        }
      }
      ASSERT(found != -1, "No task with name found.");

      msg[0] = tasks[found].tid;

      Reply(tid, msg, 1);
    } else if (type == REGISTER_AS) {
      bwprintf(COM1, "Registering %d with name %s \n", tid, msg);
      ASSERT(emptyTaskName < NUM_TASK_NAMESERVER, "Too many tasks registered with nameserver.");
      tasks[emptyTaskName].tid = (signed char)tid;
      memcpy_no_overlap_asm(msg, tasks[emptyTaskName++].name, len-2);
      msg[0] = '\0';
      Reply(tid, msg, 1);
    } else {
      ASSERT(FALSE, "Unknown NameServer Action.\n");
    }
  }
}

void startNameServerTask() {
  int nameserverTid = Create(1, nameserver_task);
  ASSERT(NAMESERVER_TID == nameserverTid, "Nameserver tid is expected.");
}

int RegisterAs(char* name) {
  int len = strlen(name);
  name[len+1] = REGISTER_AS;

  char reply;
  Send(NAMESERVER_TID, name, len+2, &reply, 1);
  return 0;
}

int WhoIs(char* name) {
  const int len = strlen(name);
  name[len+1] = WHO_IS;

  char reply;
  Send(NAMESERVER_TID, name, len+2, &reply, 1);
  return (int)reply;
}
