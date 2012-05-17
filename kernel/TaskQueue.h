#ifndef TASK_QUEUE_H_
#define TASK_QUEUE_H_

#include <task.h>

typedef struct TaskQueue {
    TaskDescriptor* begin;
    TaskDescriptor* end;
} TaskQueue;

void init_priority_queue();

TaskDescriptor* next_ready_task();
void append_task(TaskDescriptor* td);


#endif
