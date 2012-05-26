#ifndef SYSCALL_H_
#define SYSCALL_H_

#include <util.h>

typedef void (*func_t)();

#define SYSCALL_CREATE 0
#define SYSCALL_MYTID 1
#define SYSCALL_MYPARENTTID 2
#define SYSCALL_PASS 3
#define SYSCALL_REPLY 4
#define SYSCALL_SEND 5
#define SYSCALL_RECEIVE 6
#define SYSCALL_EXIT 7
#define LAST_SYSCALL 8

int Create(int priority, func_t code);

int MyTid();

int MyParentsTid();

void Pass();

void Exit();

int Send( int tid, char *msg, int msglen, char *reply, int replylen);

int Receive(int *tid, char *msg, int msglen);

int Reply( int tid, char *reply, int replylen);

#endif // SYSCALL_H_
