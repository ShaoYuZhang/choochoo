#include <Scheduler.h>
#include <TaskQueue.h>
#include <memory.h>

volatile static TaskDescriptor *currentRunningTask;

// TODO(cao): I used TaskQueue at first but it crashed.. this simple versions eem to wrok

void scheduler_init() {
  init_ready_queue();
}

volatile TaskDescriptor* scheduler_get_running() {
	return currentRunningTask;
}

void scheduler_append(volatile TaskDescriptor *td) {
  append_task(td);
}

volatile TaskDescriptor* scheduler_get() {
  return next_ready_task();
}

void scheduler_set_running(volatile TaskDescriptor* td) {
  currentRunningTask = td;
}

void scheduler_killme() {
  //TODO remove?
	//TaskDescriptor *td = currentRunningTask;
	//free_user_memory((addr)td);
}

void scheduler_move2ready() {
	ASSERT(running, "no task running to move to ready");
	scheduler_append(currentRunningTask);
}
