#ifndef MEMORY_H_
#define MEMORY_H_

#include <util.h>
#include <task.h>

// Also look at the variables in orex.ld
#define USER_MEM_START	0x300000
#define USER_MEM_END	0x1900000
// the size of user memory in bytes (64 KB)
#define STACK_SIZE 65536

// Artificial limit..
#define NUM_MAX_TASK 128

#define BYTES2WORDS(x) ((x) >> 2)
#define NEXTHIGHESTWORD(x) ((x)-(x)%4)

void mem_reset();

void* kmalloc(unsigned int size); // allocate kernel memory

addr allocate_user_memory();

void free_user_memory(addr a);

//void memcpy_no_overlap_simple(char* from, char* to, int len);
void memcpy_no_overlap_asm(char* from, char* to, int len);

#endif //MEMORY_H_
