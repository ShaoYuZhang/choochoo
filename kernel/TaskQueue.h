#ifndef TASK_QUEUE_H_
#define TASK_QUEUE_H_

#include <task.h>

typedef struct TaskQueue {
    volatile TaskDescriptor* begin;
    volatile TaskDescriptor* end;
} TaskQueue;

void init_ready_queue();

volatile TaskDescriptor* next_ready_task();

void append_task(volatile TaskDescriptor* td);

#endif
