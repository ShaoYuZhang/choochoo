#ifndef KERNEL_H_
#define KERNEL_H_

#include <task.h>
#include <syscall.h>

// Kernel version of system calls.

// Implement as needed
void handle_swi(int** sp_pointer);

void kernel_init();

void kernel_createtask(int* returnPtr, int priority, func_t code);

void kernel_mytid(int* returnVal);

void kernel_myparenttid(int* returnVal);

void kernel_pass();

void kernel_exit();

void kernel_send(int* arg0, char* arg1, int arg2);

void kernel_receive(int not_used, int tid, int msg, int msglen);

void kernel_reply(int* returnVal , int tid, char* arg2, int replylen);

void kernel_runloop();

#endif // KERNEL_H_
