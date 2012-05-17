#ifndef MEMORY_H_
#define MEMORY_H_

#include <util.h>
#include <task.h>

// Also look at the variables in orex.ld
#define USER_MEM_START	0x300000
#define USER_MEM_END	0x1900000
// the size of user memory in bytes (64 KB)
#define STACK_SIZE 65536
// this is calculated in compile time.
#define NUM_MAX_TASK ((USER_MEM_END - USER_MEM_START) / STACK_SIZE)

#define BYTES2WORDS(x) ((x) >> 2)

#define NEXTHIGHESTWORD(x) BYTES2WORDS((x) + 3)

extern int _TextStart;
extern int _TextEnd;

void mem_reset();

void* kmalloc(unsigned int size); // allocate kernel memory

addr allocate_user_memory();

void free_user_memory(addr a);

#endif //MEMORY_H_

