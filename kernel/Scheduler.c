#include <Scheduler.h>
#include <TaskQueue.h>
#include <memory.h>

volatile static TaskDescriptor *currentRunningTask;

void scheduler_init() {
  init_ready_queue();
  currentRunningTask = NULL;
}

volatile TaskDescriptor* scheduler_get_running() {
	return currentRunningTask;
}

void scheduler_append(volatile TaskDescriptor *td) {
	ASSERT(td->state == READY, "Can only append ready tasks");
  append_task(td);
}

volatile TaskDescriptor* scheduler_get() {
  return next_ready_task();
}

void scheduler_set_running(volatile TaskDescriptor* td) {
  currentRunningTask = td;
  currentRunningTask->state = ACTIVE;
}

void scheduler_move2ready() {
	ASSERT(currentRunningTask, "no task running to move to ready");
  currentRunningTask->state = READY;
	scheduler_append(currentRunningTask);
}

