#include <bwio.h>
#include <kernel.h>
#include <syscall.h>

static void task2() {
  bwprintf(COM2, "task id: %d, parent's task id: %d\n", MyTid(), MyParentsTid());
  Pass();
  bwprintf(COM2, "task id: %d, parent's task id: %d\n", MyTid(), MyParentsTid());
  bwprintf(COM2, "Second: exiting\n");
  Exit();
}

static void task1() {
  for (int i = 0; i < 4; i++) {
    int priority = 2 * (i >> 1);
    bwprintf(COM2, "Created: %d\n", Create(priority, task2));
  }
  bwprintf(COM2, "First: exiting\n");
  Exit();
}

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();

  kernel_createtask(1, task1);
	kernel_runloop();

	bwprintf(COM2, "boo..!!\n");
	return 0;
}

