#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>

void task1() {
  //startNameserver();
  int a;
  bwprintf(COM2, "1Stack:%d\n", (int)&a);
  Exit();
}

void task2() {
  int a;
  bwprintf(COM2, "2Stack:%d\n", (int)&a);
  Exit();
}

static char from[]        = "asdfghjklqwertyuio";
static char destination[] = "1234567890-2345678";

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();

  //bwputstr(COM2, from);
  //bwputstr(COM2, "\n");
  //bwputstr(COM2, destination);
  //memcpy_no_overlap_simple(from, destination, 18);
  //equal(from, destination, 16);
  //bwputstr(COM2, "copied...\n");
  //bwputstr(COM2, from);
  //bwputstr(COM2, "\n");
  //bwputstr(COM2, destination);
  kernel_createtask(3, task1);
  kernel_createtask(2, task2);
  //kernel_createtask(3, task3);
	kernel_runloop();
	return 0;
}

