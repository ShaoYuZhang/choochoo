#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <TimeServer.h>

void client();

void task1() {
  startNameServerTask();
  startTimeServerTask();

  int id1 = Create(3, client);
  int id2 = Create(4, client);
  int id3 = Create(5, client);
  int id4 = Create(6, client);

  char replyBuffer[2];
  int receiveId;

  for (int i = 0; i < 4; i++) {
    Receive(&receiveId, (char *)NULL, 0);
    if (receiveId == id1) {
      replyBuffer[0] = 10;
      replyBuffer[1] = 20;
    } else if (receiveId == id2) {
      replyBuffer[0] = 23;
      replyBuffer[1] = 9;
    } else if (receiveId == id3) {
      replyBuffer[0] = 33;
      replyBuffer[1] = 6;
    } else if (receiveId == id4) {
      replyBuffer[0] = 71;
      replyBuffer[1] = 3;
    }
    Reply(receiveId, replyBuffer, 2);
  }
  Exit();
}

void client() {
  int id = MyTid();
  int parent = MyParentsTid();
  char replyBuffer[2];
  Send(parent, (char *)NULL, 0, replyBuffer, 2);
  int delayTime = replyBuffer[0];
  int numberDelay = replyBuffer[1];

  int timeServerId = WhoIs(TIMESERVER_NAME);
  for (int i = 0; i < numberDelay; i++) {
    Delay(delayTime, timeServerId);
    bwprintf(COM2, "Tid: %d, Interval: %d, NumDelay: %d\n\r", id, delayTime, i +1);
  }

  Exit();
}

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();

  int returnVal;

  kernel_createtask((int*)&returnVal, 1, (int)task1, 0);

	kernel_runloop();
	return 0;
}
