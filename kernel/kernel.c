#include <kernel.h>
#include <util.h>
#include <task.h>
#include <interrupt.h>
#include <memory.h>
#include <syscall.h>
#include <Scheduler.h>
#include <bwio.h>

static void install_interrupt_handlers() {
	INSTALL_INTERRUPT_HANDLER(SWI_VECTOR, asm_handle_swi);
}

static unsigned int tid_counter;

void kernel_init() {
  tid_counter = 0;
	install_interrupt_handlers();
	mem_reset();
	scheduler_init();
}

void handle_swi(volatile register_set* reg) {
	volatile int *r0 = &reg->r[0];
	int request = *r0;
	int arg1 = reg->r[1];
	int arg2 = reg->r[2];

  // TODO(zhang) branch is ugly
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
		default:
    {
			ERROR("Unknown system call %d (%x)\n", request, request);
			break;
    }
	}
}

void kernel_runloop() {
	volatile TaskDescriptor *td;
	volatile register_set* reg;

	while ((td = scheduler_get())) {
		reg = &(td->registers);
    scheduler_set_running(td);
		asm_switch_to_usermode(reg);
		handle_swi(reg);
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
  //bwprintf(COM2, "SP %d\n", (unsigned int)mem );
	TaskDescriptor* td = (TaskDescriptor*)mem;
  td->id = tid_counter++;
	td->state = READY;
	td->priority = priority;
	td->parent_id = kernel_mytid();
	td->registers.r[REG_LR] = (int) Exit;
	td->registers.r[REG_PC] = (int) code;
	td->registers.spsr = 0x10;
  unsigned int tmp = (unsigned int)mem + STACK_SIZE;
  tmp = tmp - tmp%4 - 16;
	td->registers.r[REG_SP] = tmp;
  //bwprintf(COM2, "SP %d\n", td->registers.r[REG_SP]);

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
