#ifndef MEMORY_H_
#define MEMORY_H_

#include <task.h>

#define BYTES2WORDS(x) ((x) >> 2)
#define NEXTHIGHESTWORD(x) ((x)-(x)%4)

void mem_reset();

char* allocate_user_memory();

void free_user_memory(char* a);

void memcpy_no_overlap_asm(char* from, char* to, int len);

#endif //MEMORY_H_
