#ifndef TASK_H_
#define TASK_H_

typedef enum TaskState {
    ACTIVE,
    READY,
    ZOMBIE,
    SEND_BLOCK,
    RECEIVE_BLOCK,
    REPLY_BLOCK
} TaskState;

typedef struct TaskDescriptor {
		int id;
		TaskState state;
		unsigned int priority;
		int parent_id; // should this be a pointer to the parent td?
		int* sp;
		volatile struct TaskDescriptor* next;
		volatile struct TaskDescriptor* sendQ;
} TaskDescriptor;

#endif // TASK_H_
