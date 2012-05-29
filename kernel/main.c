#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <idle.h>

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

int main(int argc, char* argv[]) {
	bwioInit();
	kernel_init();

  int returnVal;

  int *timer_control = (int *)(TIMER3_BASE + CRTL_OFFSET);

  int control = *timer_control;
  control = control | CLKSEL_MASK; //USE higher freq clock
  control = control & (~MODE_MASK); // period mode
  control = control | ENABLE_MASK; // enable

  *timer_control = control;

  kernel_createtask(&returnVal, 1, timing1);
  kernel_createtask(&returnVal, 1, timing2);

	kernel_runloop();
	return 0;
}

