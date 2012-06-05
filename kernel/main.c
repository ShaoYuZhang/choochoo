#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <IoServer.h>

void client();

void task1() {
  startNameServerTask();
//  startTimeServerTask();

  int id = startIoServerTask();
  for (int i = 0; i < 5; i++) {
    Send(id, (char*)NULL, 0, (char*)NULL, 0);
  }

  bwputstr(COM1, "DONE1");

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
