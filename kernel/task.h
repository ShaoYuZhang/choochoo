#ifndef TASK_H_
#define TASK_H_

typedef enum TaskState {
    ACTIVE,
    READY,
    BLOCKED,
    ZOMBIE
} TaskState;

typedef struct TaskDescriptor {
		int id;
		TaskState state;
		unsigned int priority;
		int parent_id; // should this be a pointer to the parent td?
		int* sp;
		volatile struct TaskDescriptor* next;
} TaskDescriptor;

#endif // TASK_H_
