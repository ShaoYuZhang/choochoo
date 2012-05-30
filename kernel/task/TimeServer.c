#include "TimeServer.h"
#include <util.h>
#include <syscall.h>
#include <NameServer.h>

#define TIMER_SERVER_SIZE 16 // WARNING: must be power of 2
#define TIMER_SERVER_SIZE_MOD 15 // TIMER_SERVER_SIZE-1
#define TIME_REQUEST 0

void timernotifier_task();
void timeserver_task();

// Size optimization if needed.....
typedef struct RegisteredTask {
  char tid;
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
  // Enable timer device
  VMEM(TIMER1_BASE + CRTL_OFFSET) &= ~ENABLE_MASK; // stop timer
  VMEM(TIMER1_BASE + LDR_OFFSET)   = 2000; ///508; // Corresponds to clock frequency
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= MODE_MASK; // pre-load mode
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= CLKSEL_MASK; // 508Khz clock
  VMEM(TIMER1_BASE + CRTL_OFFSET) |= ENABLE_MASK; // start

  int counter = 0;
  int basePos = 0;
  char name[] = TIMESERVER_NAME;
  RegisterAs(name);

  // Initialize sorted list of tasks.
  taskQueueLen = 0;
  for (int i = 0; i < TIMER_SERVER_SIZE; i++) {
    taskQueue[i].tid = -1;
  }

  int notifierId = Create(0, timernotifier_task);

  // Start serving..
  for (;;) {
    int msgBuff = -1;
    int tid = -1;
    int len = Receive(&tid, (char*)&msgBuff, 4);
    ASSERT(len == 0 || len == 4, "Bad message to time server.");

    if (tid == notifierId) {
      counter += 1;
      Reply(tid, (char*)NULL, 0);

      // Reply to applicable queues...
      while (taskQueueLen) {
        if (taskQueue[basePos & TIMER_SERVER_SIZE_MOD].time <= counter) {
          Reply(taskQueue[basePos & TIMER_SERVER_SIZE_MOD].tid, NULL, 0);
          taskQueue[basePos & TIMER_SERVER_SIZE_MOD].time = -1;
          basePos = (basePos+1) & TIMER_SERVER_SIZE_MOD;
          taskQueueLen--;
        } else {
          break;
        }
      }

      if (counter%1000 == 0)
        bwprintf(COM2, "%d\n", counter);
    } else if (len == 0) {
      // Timing request
      bwprintf(COM2, "time r:%d\n", counter);
      Reply(tid, (char*)&counter, 4);
    } else {
      if (msgBuff & 0xf0000000) {
        bwputstr(COM2, "delay msg\n");
        // Delay message
        msgBuff += counter;
      } else {
        bwputstr(COM2, "delay until msg\n");
      }
      // Delay until message, dont need to add current time.
      msgBuff &= 0x00ffffff;
      bwprintf(COM2, "delay until %d\n", msgBuff);
      bwprintf(COM2, "delay msg to %d\n", tid);

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
      taskQueue[insertPoint].tid = (char)tid;
      bwprintf(COM2, "Added at %d\n", insertPoint);

      ASSERT(taskQueueLen != TIMER_SERVER_SIZE, "MAX BUFFER");
      taskQueueLen++;
    }
  } // End of serve loop
}

int startTimeServerTask() {
  return Create(1, timeserver_task);
}

void timernotifier_task() {
  int parent = MyParentsTid();

  // Enables timer interrupt.
  // TODO, move to kernel.
  int irqmask = INT_MASK(TIMER_INT_MASK);
  VMEM(VIC1 + INT_ENABLE) = irqmask;

  int counter = 0;
  for (;;) {
    AwaitEvent(0);
    Send(parent, (char*)NULL, 0, (char*)NULL, 0);
    counter++;
    if (counter % 20 == 0) {
      bwputstr(COM2, "no\n");
    }
  }
}
