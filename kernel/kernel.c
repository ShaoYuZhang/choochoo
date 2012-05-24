#include <kernel.h>
#include <util.h>
#include <task.h>
#include <interrupt.h>
#include <memory.h>
#include <syscall.h>
#include <Scheduler.h>
#include <bwio.h>

extern int _TextStart;
extern int _TextEnd;

static void install_interrupt_handlers() {
	INSTALL_INTERRUPT_HANDLER(SWI_VECTOR, asm_handle_swi);
}

static unsigned int tid_counter;
static TaskDescriptor tds[NUM_MAX_TASK];

void kernel_init() {
  tid_counter = 0;
	install_interrupt_handlers();
	mem_reset();
	scheduler_init();
}

void handle_swi(int** sp_pointer) {
  // task stack layout reminder
  // sp: r0-r12, lr, spsr
	volatile int *r0 = *sp_pointer;
	int request = *r0;
	int arg1 = (*sp_pointer)[1];
	int arg2 = (*sp_pointer)[2];
	int arg3 = (*sp_pointer)[3];
	int arg4 = (*sp_pointer)[4];
	int arg5 = (*sp_pointer)[5];

  // TODO(zhang) branch is ugly function[request](arg, arg, arg, arg);
	switch (request) {
		case SYSCALL_CREATE:
    {
			*r0 = kernel_createtask(arg1, (func_t) arg2);
      scheduler_move2ready();
			break;
    }
    case SYSCALL_MYTID:
    {
      *r0 = kernel_mytid();
      scheduler_move2ready();
      break;
    }
    case SYSCALL_MYPARENTTID:
    {
      *r0 = kernel_myparenttid();
      scheduler_move2ready();
      break;
    }
    case SYSCALL_PASS:
    {
      scheduler_move2ready();
      break;
    }
    case SYSCALL_EXIT:
    {
      kernel_exit();
      break;
    }
    case SYSCALL_SEND:
    {

    }
    case SYSCALL_RECEIVE:
    {

    }
    case SYSCALL_REPLY:
    {

    }
		default:
    {
			ERROR("Unknown system call %d (%x)\n", request, request);
			break;
    }
	}
}

void kernel_runloop() {
	volatile TaskDescriptor *td;
	int** sp_pointer;

	while ((td = scheduler_get())) {
		sp_pointer = (int**)&(td->sp);
    scheduler_set_running(td);
		asm_switch_to_usermode(sp_pointer);
		handle_swi(sp_pointer);
	}
}

int kernel_createtask(int priority, func_t code) {
	if (priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
		return -1;
	}
	unsigned int codeaddr = (unsigned int)code;

	// probably not in the text region
	if (codeaddr < (unsigned int)&_TextStart || codeaddr >= (unsigned int)&_TextEnd ) {
		return -3;
	}

  // | -------------->
  // | TD | stack ->>>>
  addr mem = allocate_user_memory();
  if (mem == NULL) {
    return -2;
  }
  volatile TaskDescriptor* td = &tds[tid_counter];
  td->id = tid_counter++;
	td->state = READY;
	td->priority = priority;
	td->parent_id = kernel_mytid();
  unsigned int tmp = (unsigned int)mem + STACK_SIZE;
  tmp = tmp - tmp%4 - 16;
  td->sp = (int *)(tmp - 4 * 14); //
  td->sp[13] = (int) code; // LR
  td->sp[14] = 0x10; //spsr

	scheduler_append(td);
	return td->id;
}

int kernel_mytid() {
	volatile TaskDescriptor *td = scheduler_get_running();
	return td ? td->id : 0xdeadbeef;
}

int kernel_myparenttid() {
	volatile TaskDescriptor *td = scheduler_get_running();
	return td ? td->parent_id : 0xdeadbeef;
}

void kernel_exit() {
	volatile TaskDescriptor *td = scheduler_get_running();
  td->state = ZOMBIE;
}

int kernel_send(int tid, char *msg, int msglen, char *reply, int replylen) {
  if (tid >= NUM_MAX_TASK){
    return -1;
  }

  if (tid >= tid_counter) {
    return -2;
  }
  volatile TaskDescriptor* receive = &tds[tid];

  if (receive->state == ZOMBIE) {
    return -2;
  }

  return 0;
}

int kernel_receive(int *tid, char *msg, int msglen) {

  return 0;
}

int kernel_reply(int tid, char* reply, int replylen) {

  return 0;
}
