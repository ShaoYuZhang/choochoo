#include "NameServer.h"
#include <memory.h>
#include <string.h>

#define NUM_TASK_NAMESERVER 20
#define MSG_LEN 15

typedef struct Task {
		signed char tid;
    char name[MSG_LEN];
} Task;

static void nameserver_task() {
  Task tasks[NUM_TASK_NAMESERVER];
  int tid = -1;
  char msg[MSG_LEN];
  int emptyTaskName = 0;

  while (1) {
    int len = Receive(&tid, msg, MSG_LEN);
    char type = msg[len-2];
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
      msg[1] = '\0';

      Reply(tid, msg, 2);
    } else if (type == REGISTER_AS) {
      //bwprintf(COM2, "Registering %d \n", tid);
      //bwprintf(COM2, "Len %d \n", len-2);
      tasks[emptyTaskName].tid = (signed char)tid;
      memcpy_no_overlap_asm(msg, tasks[emptyTaskName++].name, len-2);
      ASSERT(emptyTaskName < NUM_TASK_NAMESERVER, "Too many tasks registered with nameserver.");
      msg[0] = '\0';
      Reply(tid, msg, 1);
    } else {
      ASSERT(FALSE, "Unknown NameServer Action.\n");
    }
  }
}

void startNameserver() {
  int nameserverTid = Create(1, nameserver_task);
  ASSERT(NAMESERVER_TID == nameserverTid, "Nameserver tid is not as expected.");
}

int RegisterAs(char* name) {
  int len = strlen(name);
  name[len+1] = REGISTER_AS;
  name[len+2] = '\0';
  //bwprintf( COM2, "%d len\n\r", len);

  char reply;
  Send(NAMESERVER_TID, name, len+3, &reply, 1);
  return 0;
}

int WhoIs(char* name) {
  int len = strlen(name);
  name[len+1] = WHO_IS;
  name[len+2] = '\0';

  char reply[2];
  Send(NAMESERVER_TID, name, len+3, reply, 2);
  return (int)(reply[0]);
}
