#ifndef RTX_H_
#define RTX_H_

#include <ts7200.h>
#include <TaskDescriptor.h>

TaskDescriptor* get_running_task();
void set_running_task(TaskDescriptor* td);
TaskDescriptor* schedule();

#endif
