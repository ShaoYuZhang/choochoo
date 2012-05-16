#include <ts7200.h>
#include <TaskQueue.h>

TaskQueue taskPriorityQueues[NUM_PRIORITY];

void init_priority_queue() {
    for (int i = 0; i < NUM_PRIORITY; ++i) {
        taskPriorityQueues[i].begin = NULL;
        taskPriorityQueues[i].end = NULL;
    }
}

TaskDescriptor* next_ready_task() {
    for (int i = 0; i < NUM_PRIORITY; ++i) {
        if (taskPriorityQueues[i].begin != NULL) {
            TaskDescriptor* result = taskPriorityQueues[i].begin;
            result->next = NULL;

            taskPriorityQueues[i].begin = taskPriorityQueues[i].begin->next;
            if (taskPriorityQueues[i].begin == NULL) {
                taskPriorityQueues[i].end = NULL;
            }
            return result;
        }
    }
    return NULL;
}

void append_task(TaskDescriptor* td) {
    td->next = NULL;
    int priority = td->priority;
    if (taskPriorityQueues[priority].begin == NULL) {
        taskPriorityQueues[priority].begin = td;
        taskPriorityQueues[priority].end = td;
    } else {
        taskPriorityQueues[priority].end->next = td;
        taskPriorityQueues[priority].end = td;
    }
}
