#include <idle.h>
#include <util.h>
#include <syscall.h>

int startIdleTask() {
  return Create(LOWEST_PRIORITY, idle_task);
}
