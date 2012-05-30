#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <idle.h>

void task1() {
  startNameServerTask();
  startTimeServerTask();
  startIdleTask();

  Exit();
}

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();

  int returnVal;

  kernel_createtask((int)&returnVal, 1, task1, 0);

	kernel_runloop();
	return 0;
}

