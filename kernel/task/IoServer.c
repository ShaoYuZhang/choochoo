#include "IoServer.h"
#include <ts7200.h>
#include <util.h>
#include <syscall.h>
#include <NameServer.h>

char Getc(const int channel) {
  if (channel == COM1) {

  } else {
    ASSERT(FALSE, "Not implemented.");
  }
  return '\0';
}

void Putc(const int channel, const char c) {
  if (channel == COM1) {

  } else {
    ASSERT(FALSE, "Not implemented.");
  }
}

void com1notifier_task() {
}

void com2notifier_task() {

}

void ioserver_task() {
  char name[] = IOSERVER_NAME;
  RegisterAs(name);

  //int com1 = Create(HIGHEST_PRIORITY, com1notifier_task);

  for (;;) {
    int msgBuff = -1;
    int tid = -1;
    //int len = Receive(&tid, (char*)&msgBuff, 4);
    //Reply(tid, (char*)NULL, 0); // Reply to notifier
    bwprintf(COM2, "ASDF\n");
  }
}

int startIoServerTask() {
  return Create(1, ioserver_task);
}


