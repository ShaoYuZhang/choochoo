#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>

void task1() {
  //startNameserver();


  char *a = "Hello\n\r";
  char b[10];
  int temp = Send(1, a, 8, b, 10);
  bwprintf( COM2, "%d char replied\n\r", temp);
  bwputstr( COM2, b);
  Exit();
}

void task2() {
  int a;
  char b[10];
  int temp = Receive( &a, b, 10);
  bwprintf( COM2, "%d char received\n\r", temp);
  bwprintf( COM2, "from task %d\n\r", a);
  bwputstr( COM2, b);

  char *c = "Hey\n\r";
  Reply(0, c, 6);

  Exit();
}

void task3() {

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
  kernel_createtask(2, task1);
  kernel_createtask(2, task2);
  //kernel_createtask(3, task3);
	kernel_runloop();
	return 0;
}

