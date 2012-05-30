#include <idle.h>
#include <util.h>

static void idle_task() {
  while (1) {
    // NOTHING
  }
}


void startIdleTask() {
  int idle_tid = Create(MAX_PRIORITY, idle_task);
}
