#ifndef TASK_QUEUE_H_
#define TASK_QUEUE_H_

#define NUM_PRIORITY 10 // 0 = HIGHEST, 9 = LOWEST

#include <TaskDescriptor.h>

typedef struct TaskQueue {
    TaskDescriptor* begin;
    TaskDescriptor* end;
} TaskQueue;

void init_priority_queue();

TaskDescriptor* next_ready_task();
void append_task(TaskDescriptor* td);

#endif
