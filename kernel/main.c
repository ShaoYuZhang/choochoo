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
  startTimeServerTask();
  startIoServerTask();

  Exit();
}

int main(int argc, char* argv[]) {
	kernel_init();

  int returnVal;

  kernel_createtask((int*)&returnVal, 1, (int)task1, 0);

	kernel_runloop();
	return 0;
}
