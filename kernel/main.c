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
    PutcCOM2(id, 'u');
  }

  bwputstr(COM1, "finished putc, now getc");

  char c = GetcCOM2(id);

  bwprintf(COM1, "Returned with %c \n", c);

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
