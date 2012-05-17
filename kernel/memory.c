#include <memory.h>
#include <util.h>
#include <stack.h>
#include <Scheduler.h>
#include <syscall.h>

extern unsigned int _KernelMemStart;
static addr kernel_heap;
static stack *umpages;

void mem_reset() {
	// initialize kernel heap
	kernel_heap = &_KernelMemStart;
	// initialize user memory pages
	umpages = stack_new(TASK_LIST_SIZE);
	for (int i = TASK_LIST_SIZE - 1; i != -1; i--) {
		stack_push(umpages, (void*) (USER_MEM_START + STACK_SIZE * i));
	}
}

void* kmalloc(unsigned int size) {
	addr rv = kernel_heap;
	kernel_heap += NEXTHIGHESTWORD(size);
	ASSERT((int) kernel_heap < USER_MEM_START, "kernel heap overflow");
	return rv;
}

void* umalloc(unsigned int size) {
	volatile TaskDescriptor *td = scheduler_running();
	addr rv = td->heap;
	td->heap += NEXTHIGHESTWORD(size);
	ASSERT(((unsigned int)td->registers.r[REG_SP]) > (unsigned int)td->heap,
			"user task ran out of memory");
	return rv;
}

void* qmalloc(unsigned int size) { // requires size in bytes
	int mode = 0xdeadbeef;

	__asm(
			"mrs %[mode], cpsr" "\n\t"
			"and %[mode], %[mode], #0x1f" "\n\t"
			: [mode] "=r" (mode)
	);

	switch (mode) {
		//case 0x10: // user
	//		return malloc(size);
		case 0x13: // service
			return kmalloc(size);
		default: // not handled
			ERROR("unhandled processor mode in qmalloc");
			return NULL;
	}
}

void allocate_user_memory(TaskDescriptor *td) {
	td->heap_base = (addr) stack_pop(umpages);
	td->heap = td->heap_base;
	td->registers.r[REG_SP] = ((int) td->heap) + BYTES2WORDS(STACK_SIZE);
}

void free_user_memory(TaskDescriptor *td) {
	stack_push(umpages, td->heap_base);
}
