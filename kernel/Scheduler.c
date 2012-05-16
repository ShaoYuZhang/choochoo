#include <Scheduler.h>
#include <TaskQueue.h>

TaskDescriptor* currentRunningTask;

TaskDescriptor* get_running_task() {
    return currentRunningTask;
}

void set_running_task(TaskDescriptor* td) {
    currentRunningTask = td;
}

TaskDescriptor* schedule() {
    return next_ready_task();
}
