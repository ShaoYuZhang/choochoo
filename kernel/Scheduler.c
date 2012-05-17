#include <Scheduler.h>
#include <TaskQueue.h>
#include <memory.h>

volatile static TaskDescriptor *running;
volatile static TaskDescriptor *begin;
volatile static TaskDescriptor *end;

void scheduler_init() {
	running = NULL;
  begin = NULL;
  end = NULL;
}

volatile TaskDescriptor *scheduler_running() {
	return running;
}

void scheduler_ready(TaskDescriptor *td) {
  td->scheduleNext = NULL;
  if (begin == NULL) {
    begin = td;
    end = td;
  } else {
    end->scheduleNext = td;
    end = td;
  }
}

int scheduler_empty() {
	return (begin == NULL);
}

TaskDescriptor *scheduler_get() {
  if (begin != NULL) {
    TaskDescriptor* r = begin;
    r->scheduleNext = NULL;

    begin = begin->scheduleNext;
    if (begin == NULL) {
      end = NULL;
    }
    return r;
  }
  return NULL;
}

void scheduler_killme() {
	TaskDescriptor *td = (TaskDescriptor*) running;
	free_user_memory(td);
	td_free(td);
}

void scheduler_move2ready() {
	ASSERT(running, "no task running to move to ready");
	scheduler_ready((TaskDescriptor *) running);
}
