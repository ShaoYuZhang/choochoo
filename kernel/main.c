#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <idle.h>

void generateTimeInterrupt() {
  // Enable on device
  VMEM(TIMER1_BASE + CRTL_OFFSET) &= ~ENABLE_MASK; // stop timer
  VMEM(TIMER1_BASE + LDR_OFFSET)   = 508;
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= MODE_MASK; // pre-load mode
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= CLKSEL_MASK; // 508Khz clock
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= ENABLE_MASK; // start

  int irqmask = INT_MASK(TIMER_INT_MASK);
  // Enables timer interrupt.
  VMEM(VIC2 + INT_ENABLE) = irqmask;

  for (int i = 0; i < 2000; i++) {
    //VMEM(VIC1 + SOFTINT) = i;
   // bwprintf(COM2, "Waiting..%x %x\n", VMEM(VIC1 + SOFTINT), VMEM(VIC1 + 0));
   if (i%200 == 0) {
      bwprintf(COM2, "Waiting..\n");
   }
   // , VMEM(VIC1 + SOFTINT), VMEM(VIC1 + 0));
    //print_cpsr();
  }

  Exit();
}
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

  //int *timer_control = (int *)(TIMER3_BASE + CRTL_OFFSET);

  //int control = *timer_control;
  //control = control | CLKSEL_MASK; //USE higher freq clock
  //control = control & (~MODE_MASK); // period mode
  //control = control | ENABLE_MASK; // enable

  //*timer_control = control;

  kernel_createtask((int)&returnVal, 1, generateTimeInterrupt, 0);
//  kernel_createtask(&returnVal, 1, timing2);

	kernel_runloop();
	return 0;
}

