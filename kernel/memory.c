#include <memory.h>
#include <util.h>
#include <stack.h>
#include <Scheduler.h>
#include <syscall.h>

extern unsigned int _KernelMemStart;
static addr kernel_heap;

// List of blocks of memory available for use
static stack* memChunk;

void mem_reset() {
  // initialize kernel heap
  kernel_heap = &_KernelMemStart;

  // Create a stack of memory chunks for storing user info.
  memChunk = stack_new(NUM_MAX_TASK);
  for (int i = NUM_MAX_TASK - 1; i != -1; i--) {
    stack_push(memChunk, (void*) (USER_MEM_START + STACK_SIZE * i));
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
  return (addr) stack_pop(memChunk);
}

void free_user_memory(addr a) {
  stack_push(memChunk, a);
}
