#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <TimeServer.h>

void task1() {
  startNameServerTask();
  int timerServerTid = startTimeServerTask();

  int checkId = WhoIs(TIMESERVER_NAME);

  ASSERT(checkId  == timerServerTid, "WHAT>>>>");
  bwputstr(COM2, "okay\n");


//  for (;;) {
//    Delay(10, checkId);
//    int time = Time(checkId);
//    bwprintf(COM2, "time: %d\n", time);
//  }
//
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
