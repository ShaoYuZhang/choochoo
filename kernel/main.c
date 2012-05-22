#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>

void task1() {
  startNameserver();

}

static char from[]        = "asdfghjklqwertyuio";
static char destination[] = "1234567890-2345678";

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();


  bwputstr(COM2, from);
  bwputstr(COM2, "\n");
  bwputstr(COM2, destination);
  //memcpy_no_overlap_simple(from, destination, 18);
  equal(from, destination, 16);
  bwputstr(COM2, "copied...\n");
  bwputstr(COM2, from);
  bwputstr(COM2, "\n");
  bwputstr(COM2, destination);
  //kernel_createtask(1, task1);
	//kernel_runloop();
	return 0;
}

