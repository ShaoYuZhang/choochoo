#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <RockPaperScissorsServer.h>
#include <RockPaperScissorsClient.h>

void timing2();

void timing1() {
  for(int i= 0; i < 10; i++){
    char a[64] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    char b[64];
    unsigned int val1 = *((unsigned int *)(TIMER3_BASE + VAL_OFFSET));
    Send(1, a, 4, b, 4);
    unsigned int val2 = *((unsigned int *)(TIMER3_BASE + VAL_OFFSET));
    bwprintf( COM2, "difference %u", val1 - val2);
  }

  Exit();
}

void timing2() {
  for(int i= 0; i < 10; i++){
    int t;
    char a[64];
    char b[64] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    int temp = Receive( &t, a, 4);
    Reply(0, a, 4);
  }

  Exit();
}

void task0() {
  bwputstr(COM2, "Running task0\n");
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

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();

  int returnVal;
  kernel_createtask(&returnVal, 1, task0);
#if timing_
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
  //kernel_createtask(&returnVal, 1, timing2);
  //kernel_createtask(2, task2);
  //kernel_createtask(3, task3);
>>>>>>> eee5965ec4ed8d15813dad51ab9a4f11a15e71c9
#endif
	kernel_runloop();
	return 0;
}

