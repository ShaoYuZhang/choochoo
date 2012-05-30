#include <idle.h>
#include <syscall.h>
#include <util.h>
#include <syscall.h>

void startIdleTask() {
  Create(MAX_PRIORITY, idle_task);
}
