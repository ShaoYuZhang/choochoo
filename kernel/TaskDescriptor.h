#ifndef TASK_DESCRIPTOR_H_
#define TASK_DESCRIPTOR_H_

typedef enum TaskState
{
    ACTIVE,
    READY,
    BLOCKED,
    ZOMBIE
} TaskState;

typedef struct TaskDescriptor {
    int tid;
    int priority;
    TaskState state;
    struct TaskDescriptor* next;
} TaskDescriptor;

#endif
