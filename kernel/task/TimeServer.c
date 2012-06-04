#include "TimeServer.h"
#include <util.h>
#include <syscall.h>
#include <NameServer.h>

#define TIMER_SERVER_SIZE 8      // WARNING: must be power of 2
#define TIMER_SERVER_SIZE_MOD (TIMER_SERVER_SIZE-1)

void timernotifier_task();

// Size optimization if needed.....
typedef struct RegisteredTask {
  int tid;
  int time;
} RegisteredTask;

static RegisteredTask taskQueue[TIMER_SERVER_SIZE];
static int taskQueueLen;

int Delay(int ticks, int timeServerTid) {
  ASSERT(ticks < 0x00ffffff, "Cannot delay this much.");
  ticks |= 0xf0000000;

  return Send(timeServerTid, (char*)&ticks, 4, (char*)1, 0);
}

int Time(int timeServerTid) {
  int time = -1;
  Send(timeServerTid, (char*)1, 0, (char*)&time, 4);
  return time;
}

int DelayUntil(int ticks, int timeServerTid) {
  ASSERT(ticks < 0x00ffffff, "Canot Delay until.. this much");
  ticks |= 0x0f000000;

  return Send(timeServerTid, (char*)&ticks, 4, (char*)NULL, 0);
}

void timeserver_task() {
  int counter = 0;
  int basePos = 0;
  char name[] = TIMESERVER_NAME;
  RegisterAs(name);

  // Initialize sorted list of tasks.
  taskQueueLen = 0;
  for (int i = 0; i < TIMER_SERVER_SIZE; i++) {
    taskQueue[i].tid = -1;
  }

  int notifierId = Create(HIGHEST_PRIORITY, timernotifier_task);

  // Start serving..
  for (;;) {
    int msgBuff = -1;
    int tid = -1;
    int len = Receive(&tid, (char*)&msgBuff, 4);
    ASSERT(len == 0 || len == 4, "Bad message to time server.");

    if (tid == notifierId) {
      counter += 1;
      Reply(tid, (char*)NULL, 0); // Reply to notifier


    } else if (len == 0) {
      // Timing request
      Reply(tid, (char*)&counter, 4);
    } else {
      if (msgBuff & 0xf0000000) {
        // Delay message
        msgBuff += counter;
      }
      // Delay until message, dont need to add current time.
      msgBuff &= 0x00ffffff;

      // Insert into sorted circular list.
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
        swap = pos;
      }

      taskQueue[insertPoint].time = msgBuff;
      taskQueue[insertPoint].tid = tid;

      ASSERT(taskQueueLen != TIMER_SERVER_SIZE, "Buffer maxed out.");
      taskQueueLen++;
    }

    // Reply to applicable queues...
    while (taskQueueLen) {
      if (taskQueue[basePos & TIMER_SERVER_SIZE_MOD].time <= counter) {
        Reply(taskQueue[basePos & TIMER_SERVER_SIZE_MOD].tid, (char*)NULL, 0);
        taskQueue[basePos & TIMER_SERVER_SIZE_MOD].time = -1;
        basePos = (basePos+1) & TIMER_SERVER_SIZE_MOD;
        taskQueueLen--;
      } else {
        break;
      }
    }
  } // End of serve loop
}

int startTimeServerTask() {
  return Create(1, timeserver_task);
}

void timernotifier_task() {
  int parent = MyParentsTid();
  // Enable timer device
  VMEM(TIMER1_BASE + CRTL_OFFSET) &= ~ENABLE_MASK; // stop timer
  VMEM(TIMER1_BASE + LDR_OFFSET)   = 5080; // Corresponds to clock frequency, 10 ticks
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= MODE_MASK; // pre-load mode
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= CLKSEL_MASK; // 508Khz clock
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= ENABLE_MASK; // start

  // Enables timer interrupt.
  VMEM(VIC1 + INT_ENABLE) = 1 << TC1OI;

  for (;;) {
    AwaitEvent(TC1OI);
    Send(parent, (char*)NULL, 0, (char*)NULL, 0);
  }
}
