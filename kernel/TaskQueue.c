#include <ts7200.h>
#include <TaskQueue.h>

TaskQueue taskReadyQueues[NUM_PRIORITY];

void init_priority_queue() {
    for (int i = 0; i < NUM_PRIORITY; ++i) {
        taskReadyQueues[i].begin = NULL;
        taskReadyQueues[i].end = NULL;
    }
}

TaskDescriptor* next_ready_task() {
    for (int i = 0; i < NUM_PRIORITY; ++i) {
        if (taskReadyQueues[i].begin != NULL) {
            TaskDescriptor* result = taskReadyQueues[i].begin;
            result->next = NULL;

            taskReadyQueues[i].begin = taskReadyQueues[i].begin->next;
            if (taskReadyQueues[i].begin == NULL) {
                taskReadyQueues[i].end = NULL;
            }
            return result;
        }
    }
    return NULL;
}

void append_task(TaskDescriptor* td) {
    td->next = NULL;
    int priority = td->priority;
    if (taskReadyQueues[priority].begin == NULL) {
        taskReadyQueues[priority].begin = td;
        taskReadyQueues[priority].end = td;
    } else {
        taskReadyQueues[priority].end->next = td;
        taskReadyQueues[priority].end = td;
    }
}
