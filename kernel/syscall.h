#ifndef SYSCALL_H_
#define SYSCALL_H_

#include <util.h>

#define SYSCALL_CREATE 0
#define SYSCALL_MYTID 1
#define SYSCALL_MYPARENTTID 2
#define SYSCALL_PASS 3
#define SYSCALL_EXIT 4
#define SYSCALL_MALLOC 5



int Create(int priority, func_t code);

int MyTid();

int MyParentsTid();

void Pass();

void Exit();

void* malloc(unsigned int size);

#endif SYSCALL_H_
