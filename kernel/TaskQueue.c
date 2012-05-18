#include <ts7200.h>
#include <TaskQueue.h>
#include <Scheduler.h>
#include <bwio.h>

static TaskQueue taskReadyQueues[NUM_PRIORITY];

void init_ready_queue() {
  for (int i = 0; i < NUM_PRIORITY; ++i) {
    taskReadyQueues[i].begin = NULL;
    taskReadyQueues[i].end = NULL;
  }
}

volatile TaskDescriptor* next_ready_task() {
  for (int i = 0; i < NUM_PRIORITY; ++i) {
    if (taskReadyQueues[i].begin != NULL) {
      volatile TaskDescriptor* result = taskReadyQueues[i].begin;
      taskReadyQueues[i].begin = taskReadyQueues[i].begin->next;

      if (taskReadyQueues[i].begin == NULL) {
        taskReadyQueues[i].end = NULL;
      }

      result->next = NULL;
      return result;
    }
  }
  return NULL;
}

void append_task(volatile TaskDescriptor* td) {
  td->next = NULL;
  int priority = td->priority;
  //bwprintf( COM2, "%d\n\r", 5);
  //bwputc( COM2, 56);
  if (taskReadyQueues[priority].begin == NULL) {
    taskReadyQueues[priority].begin = td;
    taskReadyQueues[priority].end = td;
  } else {
    taskReadyQueues[priority].end->next = td;
    taskReadyQueues[priority].end = td;
  }
}
