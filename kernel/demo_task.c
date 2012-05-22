#include <bwio.h>
#include <syscall.h>

void task2() {
  bwprintf(COM2, "task id: %d, parent's task id: %d\n", MyTid(), MyParentsTid());
  Pass();
  bwprintf(COM2, "task id: %d, parent's task id: %d\n", MyTid(), MyParentsTid());
  bwprintf(COM2, "Second: exiting\n");
  Exit();
}

void task1() {
  for (int i = 0; i < 4; i++) {
    int priority = 2 * (i >> 1);
    bwprintf(COM2, "Created: %d\n", Create(priority, task2));
  }
  bwprintf(COM2, "First: exiting\n");
  Exit();
}
