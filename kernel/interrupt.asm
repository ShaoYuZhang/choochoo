.text
.align	2
.global print_cpsr
print_cpsr:
  mov r0, #1
  mrs r1, cpsr
  bl bwputr(PLT)
  mov lr, pc

.text
.align	2
.global asm_handle_swi
@ this code handles swi system call
@ r0 is request id
@ r1 is a1
@ r2 is a2
@ r2 is a3
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

	@ store lr
  sub sp, sp, #4
  str lr, [sp] @ lr

	@ store spsr
  sub sp, sp, #4
  str r1, [sp] @ spsr

  @ store lr_svc, trash lr_usr
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

.text
.align	2
.global asm_handle_hwi
@ this code handles hardware interrupt
asm_handle_hwi:
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

	@ store lr
  sub sp, sp, #4
  str lr, [sp] @ lr

	@ store spsr
  sub sp, sp, #4
  str r1, [sp] @ spsr

  @ store lr_svc, trash lr_usr
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
  add sp, sp, #4 * 16 @skip r0->r12, lr, spsr
  ldr r3, [sp, #-8] @load spsr in r3
  ldr lr, [sp, #-4] @load lr in r0

  @ switch to svc mode
  mrs r2, cpsr
  bic r2, r2, #0x1F
  orr r2, r2, #0x13
  msr cpsr_c, r2

	@ restore spsr(user mode)
	msr spsr, r3

	@ restore task registers
	ldmfd r1, {r0-r12, pc}^

.text
.align	2
.global asm_syscall
asm_syscall:
  swi 0
	mov pc, lr
