#include "NameServer.h"
#include <memory.h>
#include <string.h>

#define NUM_TASK_NAMESERVER 20

static int nameserverTid;
static char taskName[NUM_TASK_NAMESERVER][128];
static int emptyTaskName;

int equal(char* a, char* b, int l){
  for(int i = 0; i < l ; i++){
    if (a[i] != b[i] ){
      return 0;
    }
  }
  return 1;
}

static void nameserver_task() {
  int* tid = (int*)NULL;
  char msg[16];
  int maxLen = 16;
  ASSERT(0 == MyTid(), "Nameserver tid is not zero.");

  while (1) {
    int len = Receive(tid, msg, maxLen);
    char type = msg[len-2];

    if (type == WHO_IS) {
      int found = -1;
      for (int i = 0; i < NUM_TASK_NAMESERVER; i++) {
        if (equal(msg, taskName[i], maxLen)) {
          found = i; break;
        }
      }

      int* result = (int*)msg;
      *result = found;
      Reply(*tid, msg, 4);
    } else if (type == REGISTER_AS) {
      memcpy_no_overlap_asm(msg, taskName[emptyTaskName], len-2);
      msg[0] = '\0';
      Reply(*tid, msg, 1);
    } else {
      bwputstr(COM2, "Unknown aciton.");
      break;
    }
  }
}

void startNameserver() {
  nameserverTid = Create(1, nameserver_task);
}

int RegisterAs(char* name) {
  int len = strlen(name);
  name[len+1] = REGISTER_AS;
  name[len+2] = '\0';

  char reply[16];
  Send(NAMESERVER_TID, name, len+3, reply, 16);
  return *((int*)reply);
}

int WhoIs(char* name) {
  int len = strlen(name);
  name[len+1] = WHO_IS;
  name[len+2] = '\0';

  char reply[16];
  Send(NAMESERVER_TID, name, len+3, reply, 16);
  return *((int*)reply);
}
