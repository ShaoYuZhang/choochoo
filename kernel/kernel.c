#include <kernel.h>
#include <util.h>
#include <task.h>
#include <interrupt.h>
#include <memory.h>
#include <syscall.h>
#include <Scheduler.h>
#include <bwio.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <idle.h>

static void (*syscall_handler[LAST_SYSCALL])(int*, int, int, int) = {
  kernel_createtask,
  kernel_mytid,
  kernel_myparenttid,
  kernel_pass,
  kernel_reply,
  kernel_send,
  kernel_receive,
  kernel_exit,
  kernel_awaitevent
};

static unsigned int tid_counter;
static unsigned int idleTid;
static TaskDescriptor tds[NUM_MAX_TASK];
static volatile TaskDescriptor* eventWaitingTask[64];

static void install_interrupt_handlers() {
	INSTALL_INTERRUPT_HANDLER(SWI_VECTOR, asm_handle_swi);
	INSTALL_INTERRUPT_HANDLER(HWI_VECTOR, asm_handle_hwi);

  // Use IRQ
  VMEM(VIC1 + INTSELECT_OFFSET) = 0;
  VMEM(VIC2 + INTSELECT_OFFSET) = 0;
  // Clear all interrupts
  VMEM(VIC1 + INTENCLR_OFFSET) = ~0;
  VMEM(VIC2 + INTENCLR_OFFSET) = ~0;
}

// TODO test.
static void uninstall_interrupt_handlers() {
  // Disable HWI handler
	INSTALL_INTERRUPT_HANDLER(HWI_VECTOR, 0);

  // Diable timers
  VMEM(TIMER1_BASE + CRTL_OFFSET) &= ~ENABLE_MASK;
  VMEM(TIMER2_BASE + CRTL_OFFSET) &= ~ENABLE_MASK;
  VMEM(TIMER3_BASE + CRTL_OFFSET) &= ~ENABLE_MASK;

  // Clear all interrupts
  VMEM(VIC1 + INTENCLR_OFFSET) = ~0;
  VMEM(VIC2 + INTENCLR_OFFSET) = ~0;
}

void kernel_init() {
  tid_counter = 0;
	install_interrupt_handlers();
	mem_reset();
	scheduler_init();
  for (int i = 0; i < 64; i++) {
    eventWaitingTask[i] = (TaskDescriptor*) NULL;
  }

  kernel_createtask((int*)&idleTid, LOWEST_PRIORITY, (int)idle_task, 0);
  bwprintf(COM2, "haha %d idle:\n", idleTid);
}

void kernel_close() {
	uninstall_interrupt_handlers();
}

void kernel_handle_interrupt() {
  int VIC1Status = VMEM(VIC1);
  //int VIC2Status = VMEM(VIC2);

  int event = 0;
  int returnVal = 0;

  if (VIC1Status & (1 << TC1OI)) {
    VMEM(TIMER1_BASE + CLR_OFFSET) = 0;
    event = TC1OI;
  }

  volatile TaskDescriptor* subscriber = eventWaitingTask[event];
  eventWaitingTask[event] = (TaskDescriptor*)NULL;
  if (subscriber != (TaskDescriptor*)NULL) {
    *(subscriber->sp) = returnVal;
    subscriber->state = READY;
    scheduler_append(subscriber);
  }
}

void kernel_runloop() {
	volatile TaskDescriptor *td;
	int** sp_pointer;

#if PERF_CHECK
  unsigned int kernelTimeStart = (((VMEM(0x80810064) & 0xff) << 32) | VMEM(0x80810060));
  unsigned int idleTimeStart = 0;
  unsigned int idleTime = 0;
  // Clear Timer4
  VMEM(0x80810064) &= ~0x100;
  VMEM(0x80810064) |= 0x100;
#endif


	while (LIKELY((td = scheduler_get()) != (TaskDescriptor *)NULL)) {
#if PERF_CHECK
    if (idleTid == td->id) {
      idleTimeStart = (((VMEM(0x80810064) & 0xff) << 32) | VMEM(0x80810060));
    }
#endif
		sp_pointer = (int**)&(td->sp);
    scheduler_set_running(td);
		int is_interrupt = asm_switch_to_usermode(sp_pointer);
#if PERF_CHECK
    if (idleTid == td->id) {
      idleTime += (((VMEM(0x80810064) & 0xff) << 32) | VMEM(0x80810060)) - idleTimeStart;
      if (idleTime > 400000) {
        int kernelTime = (((VMEM(0x80810064) & 0xff) << 32) | VMEM(0x80810060)) - kernelTimeStart;
        bwprintf(COM2, "Kenel: %d idle:%d \n", kernelTime, idleTime);
        break;
      }
    }
#endif

    if (LIKELY(is_interrupt)) {
      scheduler_move2ready();
      kernel_handle_interrupt();
    } else {
      int *arg0 = *sp_pointer;
      int request = *arg0 & MASK_LOWER;

      ASSERT(request >= 0 && request < LAST_SYSCALL, "System call not in range.");
      (*syscall_handler[request])(*sp_pointer,
                                  (*sp_pointer)[1],
                                  (*sp_pointer)[2],
                                  (*sp_pointer)[3]);

      if (request <= SYSCALL_REPLY) {
        scheduler_move2ready();
      }

    }
	} // End kernel loop;
}


void kernel_createtask(int* returnPtr, int priority, int code, int notUsed) {
  addr mem = allocate_user_memory();
  if (mem == NULL) {
    *returnPtr = -2;
  }
  volatile TaskDescriptor* td = &tds[tid_counter];
  td->id = tid_counter++;
	td->state = READY;
	td->priority = priority;
	kernel_mytid((int*)&(td->parent_id), 0, 0, 0);
  td->next = (TaskDescriptor*)NULL;
  td->sendQ = (TaskDescriptor*)NULL;
  td->sp = (int*)(mem + STACK_SIZE) - 16; // Bottom of stack are fake register values
  td->sp[13] = code; // PC_USR
  td->sp[14] = 0x50; // spsr, enabled IRQ interrupt, disable FIQ
  td->sp[15] = (int) Exit; // LR_USR

	scheduler_append(td);
	*returnPtr = td->id;
}

void kernel_mytid(int* returnVal, int notUsed1, int notUsed2, int notUsed3) {
	volatile TaskDescriptor *td = scheduler_get_running();
	*returnVal = (td != (TaskDescriptor *)NULL ? td->id : 0xdeadbeef);
}

void kernel_myparenttid(int* returnVal, int notUsed1, int notUsed2, int notUsed3) {
	volatile TaskDescriptor *td = scheduler_get_running();
	*returnVal = (td != (TaskDescriptor *)NULL ? td->parent_id : 0xdeadbeef);
}

void kernel_exit() {
	volatile TaskDescriptor *td = scheduler_get_running();
  td->state = ZOMBIE;
}

void kernel_send(int* returnVal, int msg, int arg3, int notUsed) {
  // returnVal also holds receive tid =D
  int receiver_tid = (*returnVal & MASK_HIGHER) >> 16;
  volatile TaskDescriptor* sender = scheduler_get_running();
  volatile TaskDescriptor* receiver = &tds[receiver_tid];

  if (receiver_tid >= tid_counter || receiver->state == ZOMBIE) {
    scheduler_move2ready();
    *returnVal = -2;
    return;
  }
  ASSERT(sender->id != receiver_tid, "Sending message to self.");

  if (receiver->state == SEND_BLOCK) {
    int msglen = (arg3 & MASK_HIGHER) >> 16;
    // need to wake up receiver, so need params from reciever
    volatile int* receiver_sp = receiver->sp;
    volatile int* receiver_return = receiver_sp;
    int* receiver_tid = (int*) receiver_sp[1];
    char* receiver_msg = (char*)receiver_sp[2];
    int receiver_msglen = receiver_sp[3];
    int actual_msglen = msglen < receiver_msglen ? msglen : receiver_msglen;
    memcpy_no_overlap_asm((char*)msg, receiver_msg, actual_msglen);

    *receiver_return = actual_msglen;
    receiver->state = READY;
    *receiver_tid = sender->id;
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
}

void kernel_receive(int* notUsed, int arg1, int arg2, int msglen) {
  int*  tid = (int*)  arg1;
  char* msg = (char*) arg2;
  volatile TaskDescriptor* receiver = scheduler_get_running();
  volatile int* receiver_return = receiver->sp;
  if (receiver->sendQ != (TaskDescriptor*)NULL) {
    volatile TaskDescriptor *sender = receiver->sendQ;
    receiver->sendQ = receiver->sendQ->next;

    volatile int* sender_sp = sender->sp;
    int sender_tid = sender->id;
    char* sender_msg = (char *)sender_sp[1];
    int sender_msglen = (sender_sp[2] & MASK_HIGHER) >> 16;

    int actual_msglen = msglen < sender_msglen ? msglen : sender_msglen;
    memcpy_no_overlap_asm(sender_msg, msg, actual_msglen);
    *receiver_return = actual_msglen;
    receiver->state = READY;
    *tid = sender_tid;
    scheduler_append(receiver);
    sender->state = REPLY_BLOCK;
  } else {
    // NOTE, parameters for sending task is saved on its stack
    receiver->state = SEND_BLOCK;
  }
}

void kernel_reply(int* returnVal, int tid, int reply, int replylen) {
#if MORE_CHECKING
  if (tid >= tid_counter) {
    *returnVal = -2;
    return;
  }
#endif

  volatile TaskDescriptor* sender = &tds[tid];

  if (sender->state != REPLY_BLOCK) {
    *returnVal = -3;
    return;
  }

  volatile int* sender_sp = sender->sp;
  volatile int* sender_return = sender_sp;
  char* sender_reply = (char *)sender_sp[3];
  int sender_replylen= sender_sp[2] & MASK_LOWER;

  int actual_replylen = replylen < sender_replylen ? replylen: sender_replylen;
  memcpy_no_overlap_asm((char*)reply, sender_reply, actual_replylen);
  *sender_return = actual_replylen;
  sender->state = READY;
  scheduler_append(sender);
  *returnVal = 0;
}

void kernel_pass(){}

void kernel_awaitevent(int* returnVal, int eventType, int notUsed1, int notUsed2) {
#if MORE_CHECKING
  if (eventType >= 64 || eventType < 0) {
    *returnVal = -1;
    return;
  }
  // Another task already registered for this event.
  if (eventWaitingTask[eventType] != (TaskDescriptor*)NULL) {
    *returnVal = -4;
    return;
  }
#endif
	volatile TaskDescriptor *td = scheduler_get_running();
  td->state = EVENT_BLOCK;
  eventWaitingTask[eventType] = td;
}
