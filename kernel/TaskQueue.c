#include <ts7200.h>
#include <Scheduler.h>
#include <TaskQueue.h>
#include <bwio.h>

static TaskQueue taskReadyQueues[NUM_PRIORITY];

static int queueFullMask; // each bit represent a priority

static const int MultiplyDeBruijnBitPosition[32] =
{
  0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
  31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
}; // hack

void init_ready_queue() {
  for (int i = 0; i < NUM_PRIORITY; ++i) {
    taskReadyQueues[i].begin = NULL;
    taskReadyQueues[i].end = NULL;
  }
  queueFullMask = 0;
}

volatile TaskDescriptor* next_ready_task() {
  // hack http://stackoverflow.com/questions/757059/position-of-least-significant-bit-that-is-set
  int i = MultiplyDeBruijnBitPosition[((queueFullMask & -queueFullMask) * 0x077CB531U) >> 27]; //
  if (taskReadyQueues[i].begin != NULL) {
    volatile TaskDescriptor* result = taskReadyQueues[i].begin;
    taskReadyQueues[i].begin = taskReadyQueues[i].begin->next;

    if (taskReadyQueues[i].begin == NULL) {
      taskReadyQueues[i].end = NULL;
      queueFullMask &= (~(1 << i));
    }

    result->next = NULL;
    return result;
  }

  return NULL;
}

void append_task(volatile TaskDescriptor* td) {
  td->next = NULL;
  int priority = td->priority;
  queueFullMask |= 1 << priority;
  if (taskReadyQueues[priority].begin == NULL) {
    taskReadyQueues[priority].begin = td;
    taskReadyQueues[priority].end = td;
  } else {
    taskReadyQueues[priority].end->next = td;
    taskReadyQueues[priority].end = td;
  }
}
