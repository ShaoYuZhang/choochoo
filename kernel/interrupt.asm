.text
.align	2
.global asm_handle_swi
@ this code handles swi system call
@ r0 is request id
@ r1 is a1
@ r2 is a2
@ r3 is *args
asm_handle_swi:
	@ <) swi was raised so we are in svc mode
	@ <) sp_svc: [r0-r12, lr_svc, ..., a0 contains &td->sp]
	@ <) now lr_svc = pc_usr

  @ need some scratch registers, storing things on kernal stack
	stmfd sp!, {r0-r3}
  mov r0, sp
	mrs r1, spsr
  mov r3, lr

  @ switch to system mode
  mrs r2, cpsr
  bic r2, r2, #0x1F
  orr r2, r2, #0x1F
  msr cpsr_c, r2

	@ store spsr
  sub sp, sp, #4
  str r1, [sp] @ spsr

  @ store lr
  mov lr, r3

  @restore scratch registers for user task
  ldmfd r0, {r0-r3}

	stmfd sp!, {r0-r12, lr} @ store user registers

  mov r0, sp @ need to save sp separately later

  @ switch to svc mode
  mrs r2, cpsr
  bic r2, r2, #0x1F
  orr r2, r2, #0x13
  msr cpsr_c, r2

  add sp, sp, #4 * 4
  ldr r1, [sp] @ here lies address of td->sp
  str r0, [r1] @ update task sp, task state is completely saved

	@ restore kernel state
	ldmfd sp!, {r0-r12, pc} @ restore kernel state

@ r0 is pointer to sp (task.h) of current task descriptor
.text
.align	2
.global asm_switch_to_usermode
asm_switch_to_usermode:
	@ save kernel state on stack
	stmfd sp!, {r0-r12, lr}
	@ <) [r0] is sp_usr whose layout is like this: [r0-r12, lr_usr, spsr]
  @ <) all registers are saved, can be trashed

  @ switch to system mode
  mrs r2, cpsr
  bic r2, r2, #0x1F
  orr r2, r2, #0x1F
  msr cpsr_c, r2

  ldr sp, [r0]
  mov r1, sp @store a sp in r1
  add sp, sp, #4 * 15 @skip r0->r12, lr, spsr
  ldr r3, [sp, #-4] @load spsr in r3
  str sp, [r0] @update td->sp

  @ switch to svc mode
  mrs r2, cpsr
  bic r2, r2, #0x1F
  orr r2, r2, #0x13
  msr cpsr_c, r2

	@ restore spsr(user mode)
	msr spsr, r3

	@ restore task registers
	ldmfd r1, {r0-r12, lr}

	@ resume task
	movs pc, lr @ go to there

.text
.align	2
.global asm_syscall
asm_syscall:
	stmfd	sp!, {lr}
  swi 0
	ldmfd	sp!, {pc}
