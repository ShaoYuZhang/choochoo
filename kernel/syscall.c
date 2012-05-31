#include <syscall.h>
#include <interrupt.h>
#include <util.h>

#if MORE_CHECKING
extern int _TextStart;
extern int _TextEnd;
#endif

/*
 * Usermode implementation of the system calls.
 */

int Create(int priority, func_t code) {
#if MORE_CHECKING
  if (priority < HIGHEST_PRIORITY || priority > LOWEST_PRIORITY) {
		return -1;
	}
	unsigned int codeaddr = (unsigned int)code;

	// probably not in the text region
	if (codeaddr < (unsigned int)&_TextStart || codeaddr >= (unsigned int)&_TextEnd ) {
		return -3;
	}
#endif

	return asm_syscall(SYSCALL_CREATE, priority, (int) code, 0);
}

int MyTid() {
	return asm_syscall(SYSCALL_MYTID, 0, 0, 0);
}

int MyParentsTid() {
	return asm_syscall(SYSCALL_MYPARENTTID, 0, 0, 0);
}

void Pass() {
	asm_syscall(SYSCALL_PASS, 0, 0, 0);
}

void Exit() {
	asm_syscall(SYSCALL_EXIT, 0, 0, 0);
}

int Send( int tid, char *msg, int msglen, char *reply, int replylen) {
#if MORE_CHECKING
  if (tid >= NUM_MAX_TASK) {
    return -1;
  }
#endif
  // need to hack the system in order to fit all parameters in 4 registers
  // r1: request_type(lower) + tid(higher), r2: msg
  // r3: msglen(lower) + replylen(higher) r4: reply
  int combined1 = (tid & MASK_LOWER) << 16 | (SYSCALL_SEND & MASK_LOWER);
  int combined2 = (msglen & MASK_LOWER) << 16 | (replylen & MASK_LOWER);
  return asm_syscall(combined1, (int)msg, combined2, (int)reply);
}

int Receive(int *tid, char *msg, int msglen) {
  return asm_syscall(SYSCALL_RECEIVE, (int)tid, (int)msg, msglen);
}

int Reply( int tid, char *reply, int replylen) {
#if MORE_CHECKING
  if (tid >= NUM_MAX_TASK){
    return -1;
  }
#endif
  return asm_syscall(SYSCALL_REPLY, tid, (int)reply, replylen);
}

int AwaitEvent(int eventType){
  asm_syscall(SYSCALL_AWAITEVENT, eventType, 0, 0);
  return 1;
}
