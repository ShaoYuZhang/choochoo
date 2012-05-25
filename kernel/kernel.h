#ifndef KERNEL_H_
#define KERNEL_H_

#include <task.h>
#include <syscall.h>

// Kernel version of system calls.

// Implement as needed
void handle_swi(int** sp_pointer);

void kernel_init();

int kernel_createtask(int priority, func_t code);

int kernel_mytid();

int kernel_myparenttid();

void kernel_pass();

void kernel_exit();

int kernel_send(int tid, char *msg, int msglen, char *reply, int replylen);

void kernel_receive(int *tid, char *msg, int msglen);

int kernel_reply(int tid, int arg2, int replylen);

void kernel_runloop();

#endif // KERNEL_H_
