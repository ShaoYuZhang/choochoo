#ifndef TASK_H_
#define TASK_H_

#include <util.h>

#define REG_SP 13
#define REG_LR 14
#define REG_PC 15

typedef enum TaskState {
    ACTIVE,
    READY,
    BLOCKED,
    ZOMBIE
} TaskState;

typedef struct register_set {
		int spsr;
		int r[16]; // r0-r12, sp, lr, pc
} register_set;

typedef struct TaskDescriptor {
		int id;
		TaskState state;
		unsigned int priority;
		int parent_id; // should this be a pointer to the parent td?
		register_set registers; // r0-r12, sp and lr
		volatile struct TaskDescriptor* next;
} TaskDescriptor;

#endif // TASK_H_