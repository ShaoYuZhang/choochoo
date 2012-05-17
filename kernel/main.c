#include <bwio.h>
#include <kernel.h>
#include <syscall.h>

static void task1() {
	bwprintf(COM2, "First: exiting\n");
}

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();

  kernel_createtask(1, task1);
	kernel_runloop();

	bwprintf(COM2, "worskkk..!!\n");
	return 0;
}

