#include <kernel.h>
#include <util.h>
#include <task.h>
#include <interrupt.h>
#include <memory.h>
#include <syscall.h>
#include <Scheduler.h>
#include <bwio.h>

static void (*syscall_handler[8])(const int, const int, const int, const int) = {
  kernel_createtask,
  kernel_mytid,
  kernel_myparenttid,
  kernel_pass,
  kernel_reply,
  kernel_send,
  kernel_receive,
  kernel_exit
};

static unsigned int tid_counter;
static TaskDescriptor tds[NUM_MAX_TASK];

static void handle_hwi() {
  bwputstr(COM2, "hardware interrupt\n");
  VMEM(VIC1 + SOFTINTCLEAR) = 0;
}

static void install_interrupt_handlers() {
	INSTALL_INTERRUPT_HANDLER(SWI_VECTOR, asm_handle_swi);
	INSTALL_INTERRUPT_HANDLER(HWI_VECTOR, handle_hwi);

  // Diable timers
//  VMEM(TIMER1_BASE + CRTL_OFFSET) &= ~ENABLE_MASK;
//  VMEM(TIMER2_BASE + CRTL_OFFSET) &= ~ENABLE_MASK;
//  VMEM(TIMER3_BASE + CRTL_OFFSET) &= ~ENABLE_MASK;

  // Use IRQ
  VMEM(VIC1 + INTSELECT_OFFSET) = 0;
  // Clear all interrupts
  VMEM(VIC1 + INTENCLR_OFFSET) = ~0;
  VMEM(VIC1 + INTENCLR_OFFSET) = ~0;
}

void kernel_init() {
  tid_counter = 0;
	install_interrupt_handlers();
	mem_reset();
	scheduler_init();
}

void kernel_runloop() {
	volatile TaskDescriptor *td;
	volatile int** sp_pointer;

  // TODO: when all tasks blocked.. this exits.
  //       should be when all task are in ZOMBIE
	while ((td = scheduler_get()) != (TaskDescriptor *)NULL) {
		sp_pointer = (int**)&(td->sp);
    scheduler_set_running(td);
		asm_switch_to_usermode(sp_pointer);

    int *arg0 = *sp_pointer;
    int request = *arg0 & MASK_LOWER;

    bwprintf(COM2, "kernel mode.");
    ASSERT(request >= 0 && request < LAST_SYSCALL, "System call not in range.");
    (*syscall_handler[request])(*sp_pointer, (*sp_pointer)[1], (*sp_pointer)[2], (*sp_pointer)[3]);

    if (request <= SYSCALL_REPLY) {
      scheduler_move2ready();
    }
	}
}

void kernel_createtask(int* returnPtr, int priority, func_t code) {
  // | -------------->
  // | TD | stack ->>>>
  addr mem = allocate_user_memory();
  if (mem == NULL) {
    *returnPtr = -2;
  }
  volatile TaskDescriptor* td = &tds[tid_counter];
  td->id = tid_counter++;
	td->state = READY;
	td->priority = priority;
	kernel_mytid(&(td->parent_id));
  td->next = (TaskDescriptor*)NULL;
  td->sendQ = (TaskDescriptor*)NULL;
  td->sp = (unsigned int*)(mem + STACK_SIZE) - 16; // Bottom of stack are fake register values
  td->sp[13] = (int) code; // PC_USR
  td->sp[14] = 0x10; // spsr, enabled interrupt
  td->sp[15] = (int) Exit; // LR_USR

	scheduler_append(td);
	*returnPtr = td->id;
}

void kernel_mytid(int* returnVal) {
	volatile TaskDescriptor *td = scheduler_get_running();
	*returnVal = (td != (TaskDescriptor *)NULL ? td->id : 0xdeadbeef);
}

void kernel_myparenttid(int* returnVal) {
	volatile TaskDescriptor *td = scheduler_get_running();
	*returnVal = (td != (TaskDescriptor *)NULL ? td->parent_id : 0xdeadbeef);
}

void kernel_exit() {
	volatile TaskDescriptor *td = scheduler_get_running();
  td->state = ZOMBIE;
}

void kernel_send(int* arg0, char *msg, int arg2) {
  int receiver_tid = (*arg0 & MASK_HIGHER) >> 16;
  volatile TaskDescriptor* sender = scheduler_get_running();
  volatile TaskDescriptor* receiver = &tds[receiver_tid];

  if (receiver_tid >= tid_counter || receiver->state == ZOMBIE) {
    scheduler_move2ready();
    *arg0 = -2;
    return;
  }
  ASSERT(sender->id != receiver_tid, "Sending message to self.");

  if (receiver->state == SEND_BLOCK) {
    int msglen = (arg2 & MASK_HIGHER) >> 16;
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

void kernel_receive(int not_used, int arg1, int arg2, int msglen) {
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

void kernel_reply(int* returnVal, int tid, char* reply, int replylen) {
  if (tid >= tid_counter) {
    *returnVal = -2;
    return;
  }

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
  memcpy_no_overlap_asm(reply, sender_reply, actual_replylen);
  *sender_return = actual_replylen;
  sender->state = READY;
  scheduler_append(sender);
  *returnVal = 0;
}

void kernel_pass(){
}
