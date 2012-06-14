#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <Train.h>
#include <CommandDecoder.h>
#include <IoServer.h>
#include <UserInterface.h>

void client();

void task1() {
  startNameServerTask();
  startTimeServerTask();
  startIoServerTask();
  startTrainControllerTask();
  startUserInterfaceTask();
  startCommandDecoderTask();

  Exit();
}

int main(int argc, char* argv[]) {
	kernel_init();

  int returnVal;

  kernel_createtask((int*)&returnVal, 15, (int)task1, 0);

	kernel_runloop();
	return 0;
}
