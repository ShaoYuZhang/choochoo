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
	int request = *r0 & MASK_LOWER;
	int arg1 = (*sp_pointer)[1];
	int arg2 = (*sp_pointer)[2];
	int arg3 = (*sp_pointer)[3];

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
      int tid = (*r0 & MASK_HIGHER) >> 16;
      int msglen = (arg2 & MASK_HIGHER) >> 16;
      int replylen = arg2 & MASK_LOWER;
      *r0 = kernel_send(tid, (char *)arg1, msglen, (char *)arg3, replylen);
      break;
    }
    case SYSCALL_RECEIVE:
    {
      kernel_receive((int*)arg1, (char*)arg2, arg3);
      break;
    }
    case SYSCALL_REPLY:
    {
      *r0 = kernel_reply(arg1, (char *)arg2, arg3);
      scheduler_move2ready();
      break;
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

  // TODO: when all tasks blocked.. this exits.
  //       should be when all task are in ZOMBIE
	while ((td = scheduler_get()) != (TaskDescriptor *)NULL) {
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
  td->next = (TaskDescriptor*)NULL;
  td->sendQ = (TaskDescriptor*)NULL;
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
	return td != (TaskDescriptor *)NULL ? td->id : 0xdeadbeef;
}

int kernel_myparenttid() {
	volatile TaskDescriptor *td = scheduler_get_running();
	return td != (TaskDescriptor *)NULL ? td->parent_id : 0xdeadbeef;
}

void kernel_exit() {
	volatile TaskDescriptor *td = scheduler_get_running();
  td->state = ZOMBIE;
}

int kernel_send(int reciever_tid, char *msg, int msglen, char *reply, int replylen) {
  volatile TaskDescriptor *sender = scheduler_get_running();

  int return_val = 0 ;
  if (reciever_tid >= NUM_MAX_TASK){
    return_val = -1;
  }

  if (reciever_tid >= tid_counter) {
    return_val = -2;
  }
  volatile TaskDescriptor* receiver = &tds[reciever_tid];
  ASSERT(sender->id != reciever_tid, "Sending message to self.");

  if (receiver->state == ZOMBIE) {
    return_val = -2;
  }

  if (return_val != 0) {
    scheduler_move2ready();
    return return_val;
  }

  if (receiver->state == SEND_BLOCK) {
    // need to wake up receiver, so need params from reciever
    volatile int* receiver_sp = receiver->sp;
    volatile int* receiver_return = receiver_sp;
    int* receiver_tid = (int*) receiver_sp[1];
    char* receiver_msg = (char*)receiver_sp[2];
    int receiver_msglen = receiver_sp[3];
    int actual_msglen = msglen < receiver_msglen ? msglen : receiver_msglen;
    memcpy_no_overlap_asm(msg, receiver_msg, actual_msglen);

    *receiver_return = actual_msglen;
    receiver->state = READY;
    *receiver_tid = sender->id;
    bwprintf(COM2, "Sending block: %d", sender->id);
    scheduler_append(receiver);
    sender->state = REPLY_BLOCK;
  } else {
    volatile TaskDescriptor* volatile * currentTD= &(receiver->sendQ);
    while (*currentTD != (TaskDescriptor*)NULL) {
      currentTD = &((*currentTD)->next);
    }
    *currentTD = sender;
    sender->state = RECEIVE_BLOCK;
  }

  // note: will get overwritten later
  return 0;
}

void kernel_receive(int *tid, char *msg, int msglen) {
  volatile TaskDescriptor *receiver = scheduler_get_running();
  volatile int* receiver_return = receiver->sp;
  if (receiver->sendQ != (TaskDescriptor*)NULL) {
    volatile TaskDescriptor *sender = receiver->sendQ;
    receiver->sendQ = receiver->sendQ->next;

    volatile int* sender_sp = sender->sp;
    int sender_tid = (sender_sp[0] & MASK_HIGHER) >> 16;
    char* sender_msg = (char *)sender_sp[1];
    int sender_msglen = (sender_sp[2] & MASK_HIGHER) >> 16;

    int actual_msglen = msglen < sender_msglen ? msglen : sender_msglen;
    memcpy_no_overlap_asm(sender_msg, msg, actual_msglen);
    *receiver_return = actual_msglen;
    receiver->state = READY;
    *tid = sender_tid;
    bwprintf(COM2, "Receive block: %d", sender_tid);
    scheduler_append(receiver);
    sender->state = REPLY_BLOCK;
  } else {
    receiver->state = SEND_BLOCK;
  }
}

int kernel_reply(int tid, char* reply, int replylen) {
  if (tid >= NUM_MAX_TASK){
    return -1;
  }

  if (tid >= tid_counter) {
    return -2;
  }

  volatile TaskDescriptor* sender = &tds[tid];

  if (sender->state != REPLY_BLOCK) {
    return -3;
  }

  volatile int* sender_sp = sender->sp;
  volatile int* sender_return = sender_sp;
  char* sender_reply = (char *)sender_sp[3];
  int sender_replylen= sender_sp[2] & MASK_LOWER;

  int actual_replylen = replylen < sender_replylen ? replylen: sender_replylen;
  memcpy_no_overlap_asm(reply, sender_reply, actual_replylen);
  *sender_return = actual_replylen;
  sender->state = READY;
  scheduler_append(sender);
  return 0;
}
