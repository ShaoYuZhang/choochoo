#include <task.h>
#include <memory.h>

static struct _tag_TaskDescriptor_list {
	//TaskDescriptor head_taken;
	TaskDescriptor head_free;
	TaskDescriptor td[TASK_LIST_SIZE];
} TaskDescriptors;

#define TD_REMOVE(td) { \
	(td)->prev->next = (td)->next; \
	(td)->next->prev = (td)->prev; \
}

#define TD_APPEND(ref, td) { \
	(td)->prev = (ref); \
	(td)->next = (td)->prev->next; \
	(td)->prev->next = (td); \
	(td)->next->prev = (td); \
}

void td_init() {
	// initialize free queue
	TaskDescriptor *head_free = &TaskDescriptors.head_free;

	head_free->prev = head_free;
	head_free->next = head_free;

	// put the free descriptors in the free queue
	TaskDescriptor *td = TaskDescriptors.td;

	for (int i = TASK_LIST_SIZE - 1; i != -1; --i, ++td) {
		td->id = i;
		TD_APPEND(head_free, td);
	}
}

TaskDescriptor *td_new() {
	TaskDescriptor *td = TaskDescriptors.head_free.next;

	if (td == td->next) {
		return NULL;
	}

	TD_REMOVE(td);
	return td;
}

void td_free(TaskDescriptor *td) {
	td->id += 1 << 16;
	if (td->id >= 0) {
		TD_REMOVE(td);
		TD_APPEND(&TaskDescriptors.head_free, td);
	}
}

TaskDescriptor *td_find(unsigned int id) {
	TaskDescriptor *td = &TaskDescriptors.td[id & 0x0000FFFF];
	return td->id == id ? td : NULL;
}
