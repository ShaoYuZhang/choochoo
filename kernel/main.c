#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <RockPaperScissorsServer.h>
#include <RockPaperScissorsClient.h>

void timing2();

void timing1() {
  char a[64] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  char b[64];
  unsigned int val1 = *((unsigned int *)(TIMER3_BASE + VAL_OFFSET));
  Send(1, a, 64, b, 64);
  unsigned int val2 = *((unsigned int *)(TIMER3_BASE + VAL_OFFSET));
  bwprintf( COM2, "difference %u", val1 - val2);

  Exit();
}

void timing2() {
  int t;
  char a[64];
  char b[64] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  int temp = Receive( &t, b, 64);
  Reply(0, b, 64);

  Exit();
}

void task2();

void task0() {
  bwputstr(COM2, "Start name\n");
  startNameserver();
  startServerRPS();
  startClientsRPS();

  //bwputstr(COM2, "register\n\r");
  //char* NAME = "TASK0\0\0\0";
  //int reply = RegisterAs(NAME);
  //reply = WhoIs(NAME);
  //bwprintf(COM2, "%d who1 is \n\r", reply);

  //int tid2 = Create(2, task2);
  //bwprintf(COM2, "%d tid \n\r", tid2);
  //reply = WhoIs("TASK2");
  //bwprintf(COM2, "%d who2 is \n\r", reply);

  //char *a = "Hello\n\r";
  //char b[10];
  //int temp = Send(1, a, 8, b, 10);
  //bwprintf( COM2, "%d char replied\n\r", temp);
  //bwputstr( COM2, b);
  Exit();
}

void task2() {
  char* NAME = "TASK2\0\0\0";
  int reply = RegisterAs(NAME);

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

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();

  int *timer_control = (int *)(TIMER3_BASE + CRTL_OFFSET);

  int control = *timer_control;
  control = control | CLKSEL_MASK; //USE higher freq clock
  control = control & (~MODE_MASK); // period mode
  control = control | ENABLE_MASK; // enable

  *timer_control = control;

  //bwputstr(COM2, from);
  //bwputstr(COM2, "\n");
  //bwputstr(COM2, destination);
  //memcpy_no_overlap_simple(from, destination, 18);
  //equal(from, destination, 16);
  //bwputstr(COM2, "copied...\n");
  //bwputstr(COM2, from);
  //bwputstr(COM2, "\n");
  //bwputstr(COM2, destination);
  int returnVal;
  kernel_createtask(&returnVal, 2, task0);
  //kernel_createtask(&returnVal, 1, timing1);
  //kernel_createtask(&returnVal, 2, timing2);
  //kernel_createtask(2, task2);
  //kernel_createtask(3, task3);
	kernel_runloop();
	return 0;
}

