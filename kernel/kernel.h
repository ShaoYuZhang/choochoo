#ifndef KERNEL_H_
#define KERNEL_H_

#include <task.h>

// Kernel version of system calls.

// Implement as needed
void handle_swi(register_set *reg);

void kernel_init();

int kernel_createtask(int priority, func_t code);

int kernel_mytid();

int kernel_myparenttid();

void *kernel_malloc(unsigned int size);

void kernel_runloop();

#endif // KERNEL_H_
