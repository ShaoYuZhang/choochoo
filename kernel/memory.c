#include <memory.h>
#include <util.h>
#include <Scheduler.h>
#include <syscall.h>

extern unsigned int _KernelMemStart;
static addr kernel_heap;

// List of blocks of memory available for use
static TaskDescriptor* freeTaskBlocks[NUM_MAX_TASK];
static int freeTaskBlocksCount;

void mem_reset() {
  freeTaskBlocksCount = NUM_MAX_TASK-1;
  // Initialize kernel heap
  kernel_heap = &_KernelMemStart+2;
  kernel_heap = kernel_heap - (unsigned int)kernel_heap%4 + 4;

  // Create a stack of memory chunks for storing user info.
  for (int i = 0; i < NUM_MAX_TASK; i++) {
    freeTaskBlocks[i] = (TaskDescriptor*) (USER_MEM_START + STACK_SIZE * i);
  }
}

// Kernel space malloc
void* kmalloc(unsigned int size) {
  addr rv = kernel_heap;
  kernel_heap += NEXTHIGHESTWORD(size);
  ASSERT((int) kernel_heap < USER_MEM_START, "kernel heap overflow");
  return rv;
}

addr allocate_user_memory() {
  if (freeTaskBlocksCount == -1) {
    return NULL;
  } else {
    return (addr) freeTaskBlocks[freeTaskBlocksCount--];
  }
}

void free_user_memory(addr a) {
  freeTaskBlocks[++freeTaskBlocksCount] = (TaskDescriptor*)a;
}
