#include <syscall.h>
#include <util.h>

/*
 * Usermode implementation of the system calls.
 */
static int syscall(int reqid, int a1, int a2, int a3) {
	int rv = 0xdeadbeef;
	__asm(
			"swi 0" "\n\t"
			"mov %[result], r0" "\n\t"
			: [result] "=r" (rv)
			: [reqid] "r" (reqid), [a1] "r" (a1), [a2] "r" (a2), [a3] "r" (a3)
			: "r0", "r1", "r2", "r3"
	);
	return rv;
}

int Create(int priority, func_t code) {
	return syscall(SYSCALL_CREATE, priority, (int) code, NULL);
}

int MyTid() {
	return syscall(SYSCALL_MYTID, NULL, NULL, NULL);
}

int MyParentsTid() {
	return syscall(SYSCALL_MYPARENTTID, NULL, NULL, NULL);
}

void Pass() {
	syscall(SYSCALL_PASS, NULL, NULL, NULL);
}

void Exit() {
	syscall(SYSCALL_EXIT, NULL, NULL, NULL);
}

int Send( int tid, char *msg, int msglen, char *reply, int replylen){
  // need to hack the system in order to fit all parameters in 4 registers
  // r1: request_type(lower) + tid(higher), r2: msg
  // r3: msglen(lower) + replylen(higher) r4: reply
  int combined1 = (tid & MASK_LOWER)<< 16 | (SYSCALL_SEND & MASK_LOWER);
  int combined2 = (msglen & MASK_LOWER) << 16 | (replylen & MASK_LOWER);
  return syscall(combined1, (int)msg, combined2, (int)reply);
}

int Receive(int *tid, char *msg, int msglen) {
  return syscall(SYSCALL_RECEIVE, (int)tid, (int)msg, msglen);
}

int Reply( int tid, char *reply, int replylen) {
  return syscall(SYSCALL_REPLY, tid, (int)reply, replylen);
}

