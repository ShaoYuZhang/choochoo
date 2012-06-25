#include <memory.h>
#include <bwio.h>
#include <util.h>
#include <Scheduler.h>
#include <syscall.h>

// List of blocks of memory available for use
static TaskDescriptor* freeTaskBlocks[NUM_MAX_TASK];
static int freeTaskBlocksCount;

void mem_reset() {
  freeTaskBlocksCount = NUM_MAX_TASK-1;

  // Create a stack of memory chunks for storing user info.
  for (int i = 0; i < NUM_MAX_TASK; i++) {
    freeTaskBlocks[i] = (TaskDescriptor*) (USER_MEM_START + STACK_SIZE * i);
  }
}

addr allocate_user_memory() {
  if (freeTaskBlocksCount == -1) {
    return NULL;
  } else {
    addr tmp = (addr) freeTaskBlocks[freeTaskBlocksCount--];
    return tmp;
  }
}

void free_user_memory(addr a) {
  freeTaskBlocks[++freeTaskBlocksCount] = (TaskDescriptor*)a;
}

void memcpy_no_overlap_asm(char* from, char* to, int len) {
  for (int i = 0; i < len; i++){
    to[i] = from[i];
  }
}

void* memcpy(char* to, char* from, int len) {
  for (int i = 0; i < len; i++){
    to[i] = from[i];
  }
  return to;
}
