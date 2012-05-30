#include "TimeServer.h"
#include <util.h>
#include <syscall.h>
#include <NameServer.h>

#define TIMER_SERVER_SIZE 16 // WARNING: must be power of 2
#define TIMER_SERVER_SIZE_MOD 15
#define TIME_REQUEST 0

// Size optimization if needed.....
typedef struct RegisteredTask {
  char tid;
  int time;
} RegisteredTask;

static int timeServerTid;
static RegisteredTask taskQueue[TIMER_SERVER_SIZE];
static int taskQueueLen;

int Delay(int ticks) {
  ASSERT(ticks < 0x00ffffff, "Cannot delay this much");
  ticks |= 0xf0000000;

  return Send(timeServerTid, (char*)&ticks, 4, (char*)1, 0);
}

int Time() {
  int time = -1;
  Send(timeServerTid, (char*)1, 0, (char*)&time, 4);
  return time;
}

int DelayUntil(int ticks) {
  ASSERT(ticks < 0x00ffffff, "Canot Delay until.. this much");
  ticks |= 0x0f000000;

  return Send(timeServerTid, (char*)&ticks, 4, (char*)NULL, 0);
}

void timeserver_task() {
  // Enable timer device
  VMEM(TIMER1_BASE + CRTL_OFFSET) &= ~ENABLE_MASK; // stop timer
  VMEM(TIMER1_BASE + LDR_OFFSET)   = 508; // Corresponds to clock frequency
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= MODE_MASK; // pre-load mode
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= CLKSEL_MASK; // 508Khz clock
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= ENABLE_MASK; // start

  int counter = 0;
  int basePos = 0;

  // Initialize sorted list of tasks.
  taskQueueLen = 0;
  for (int i = 0; i < TIMER_SERVER_SIZE; i++) {
    taskQueue[i].tid = -1;
  }

  // Enables timer interrupt.
  int irqmask = INT_MASK(TIMER_INT_MASK);
  VMEM(VIC1 + INT_ENABLE) = irqmask;

  int notifierId = Create(1, timernotifier_task);

  // Start serving..
  for (;;) {
    int msgBuff = -1;
    int tid = -1;
    bwputstr(COM2, "h");
    //int len = Receive(&tid, (char*)&msgBuff, 4);
    ASSERT(len == 0 || len == 4, "Bad message to time server.");

    if (tid == notifierId) {
      counter += 1;

#if 0
      // Reply to applicable queues...
      while (taskQueueLen) {
        if (taskQueue[basePos & TIMER_SERVER_SIZE_MOD].time <= counter) {
          //Reply(tid, NULL, 0);
          //printf("POP %d\n", taskQueue[basePos & TIMER_SERVER_SIZE_MOD].time);
          taskQueue[basePos & TIMER_SERVER_SIZE_MOD].time = -1;

          basePos = (basePos+1) & TIMER_SERVER_SIZE_MOD;
          taskQueueLen--;
        } else {
          break;
        }
      }
#endif
      bwprintf(COM2, "%d\n", counter);
    } else if (len == 0) {
      ASSERT(0, "WHAT");
      // Timing request
      Reply(tid, (char*)&counter, 4);
    } else {
      ASSERT(0, "WHAT");
      if (msgBuff | 0xf0000000) {
        // Delay message
        msgBuff += counter;
      }
      // Delay until message, dont need to add current time.
      msgBuff &= 0x00ffffff;

      // NOTE: shazhang think it is unlikely delay until will have its time passed.
      // If it does happen. it will be picked up at next ms tick.
      // Insert into sorted circular buffer.
      int insertPoint = (basePos + taskQueueLen) & TIMER_SERVER_SIZE_MOD;
      while (1) {
        const int checkPos = (insertPoint-1) & TIMER_SERVER_SIZE_MOD;
        if (msgBuff < taskQueue[checkPos].time) {
          insertPoint = checkPos;
        } else {
          break;
        }
      }

      int swap = (basePos + taskQueueLen) & TIMER_SERVER_SIZE_MOD;
      while (swap != insertPoint) {
        const int pos = (swap-1) & TIMER_SERVER_SIZE_MOD;
        taskQueue[swap].time = taskQueue[pos].time;
        taskQueue[swap].tid = taskQueue[pos].tid;
        swap =  pos;
      }

      taskQueue[insertPoint].time = msgBuff;
      taskQueue[insertPoint].tid = msgBuff;

      ASSERT(taskQueueLen != TIMER_SERVER_SIZE, "MAX BUFFER");
      taskQueueLen++;
    }
  } // End of serve loop
}

int createTimerserver() {
  timeServerTid = Create(1, timeserver_task);
  return timeServerTid;
}

void timernotifier_task() {
  int tid = MyParentsTid();

  int counter = 0;
  for (;;) {
    AwaitEvent(0);
    //Send(tid, (char*)NULL, 0, (char*)NULL, 0);
    if (counter % 1000 == 0) {
      bwputstr(COM2, "no\n");
    }
  }
}
