#include "TimeServer.h"
#include <util.h>
#include <syscall.h>
#include <NameServer.h>
#include <IoHelper.h>

#define TIMER_SERVER_SIZE 8      // WARNING: must be power of 2
#define TIMER_SERVER_SIZE_MOD (TIMER_SERVER_SIZE-1)

static void timernotifier_task();

// Size optimization if needed.
typedef struct RegisteredTask {
  int tid;
  int time;
  struct RegisteredTask* next;
} RegisteredTask;

static RegisteredTask taskStack[TIMER_SERVER_SIZE];

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

static void timeserver_task() {
  int numTick= 0;
  char name[] = TIMESERVER_NAME;
  RegisterAs(name);

  // Initialize stack of delay tasks.
  for (int i = 0; i < TIMER_SERVER_SIZE-1; i++) {
    RegisteredTask* t = &(taskStack[i]);
    t->next = &(taskStack[i+1]);
    t->tid = -1;
  }
  {
    RegisteredTask* t = &(taskStack[TIMER_SERVER_SIZE-1]);
    t->next = (RegisteredTask*) -1;
    t->tid = -1;
  }
  RegisteredTask* taskSlots = &(taskStack[0]);
  RegisteredTask* tasks = (RegisteredTask*)-1;

  int notifierId = Create(HIGHEST_PRIORITY, timernotifier_task);

  // Start serving..
  for (;;) {
    int msgBuff = -1;
    int tid = -1;
    int len = Receive(&tid, (char*)&msgBuff, 4);
    ASSERT(len == 0 || len == 4, "Bad message to time server.");

    if (tid == notifierId) {
      numTick += 1;
      Reply(tid, (char*)NULL, 0); // Reply to notifier
    } else if (len == 0) {
      // Timing request
      Reply(tid, (char*)&numTick, 4);
    } else {
      if (msgBuff & 0xf0000000) {
        // Delay message
        msgBuff += numTick;
      }
      // Delay until message, dont need to add current time.
      msgBuff &= 0x00ffffff;

      // Insert into linked list
      RegisteredTask* current = tasks;
      RegisteredTask* previous = (RegisteredTask*)-1;
      while (current != (RegisteredTask*)-1 && (msgBuff > current->time)) {
        previous = current;
        current = current->next;
      }

      // Insert right before t1, which is t2
      RegisteredTask* newTask = taskSlots;
      taskSlots = taskSlots->next;
      //ASSERT(newTask != (RegisteredTask*)-1);
      if (previous != (RegisteredTask*)-1) {
        previous->next = newTask;
      } else {
        tasks = newTask;
      }
      newTask->next = current;
      newTask->time = msgBuff;
      newTask->tid  = tid;
    }

    // Reply to applicable queues...
    while (tasks != (RegisteredTask*)-1 && tasks->time <= numTick) {
      Reply(tasks->tid, (char*)NULL, 0);
      RegisteredTask* unusedTask = tasks;
      tasks = tasks->next;
      unusedTask->next = taskSlots;
      taskSlots = unusedTask;
    }
  } // End of serve loop
}

int startTimeServerTask() {
  return Create(1, timeserver_task);
}

static void timernotifier_task() {
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
