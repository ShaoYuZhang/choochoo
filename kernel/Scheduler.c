#include <Scheduler.h>
#include <TaskQueue.h>
#include <memory.h>
#include <IoHelper.h>

volatile static TaskDescriptor *currentRunningTask;

void scheduler_init() {
  init_ready_queue();
  currentRunningTask = (TaskDescriptor*)NULL;
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
	ASSERT(currentRunningTask, "No task running to move to ready");
  currentRunningTask->state = READY;
	scheduler_append(currentRunningTask);
}

