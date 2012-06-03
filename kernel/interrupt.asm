.text
.align	2
.global print_cpsr
print_cpsr:
  stmfd sp!, {r0, r1, lr}
  mov r0, #1
  mrs r1, cpsr
  bl bwputr(PLT)
  ldmfd sp!, {r0, r1, pc}

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

  @ pick out some important registers
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
	ldmfd sp!, {r0-r12, lr} @ restore kernel state

  @ put in return code to indicate it's coming from swi
  mov r0, #0

  @ return 
  mov pc, lr

.text
.align	2
.global asm_handle_hwi
@ this code handles hardware interrupts
@ it's actually almost the same as asm_handle_swi
asm_handle_hwi:
	@ <) interrupt happened so we are in irq mode
	@ <) sp_svc: [r0-r12, lr_svc, ..., a0 contains &td->sp]
  @ <) sp_irq: nothing useful
	@ <) now lr_irq = pc_usr

  @ switch to svc mode, trashing sp_irq
  mrs sp, cpsr
  bic sp, sp, #0x1F
  orr sp, sp, #0x13
  msr cpsr_c, sp

  @ need some scratch registers, storing things on kernal stack
	stmfd sp!, {r0-r3}

  @ pick out kernel_sp
  mov r0, sp

  @ switch back to irq mode, 
  mrs r2, cpsr
  bic r2, r2, #0x1F
  orr r2, r2, #0x12
  msr cpsr_c, r2

  @ pick out more registers
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

  @ store lr_irq, trash lr_usr
  @ note, for some stupid reason, return address is lr -4 and not lr for irq mode.....
  sub lr, r3, #4 

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
	ldmfd sp!, {r0-r12, lr} @ restore kernel state

  @ put in return code to indicate it's coming from interrupt
  mov r0, #1

  @ return 
  mov pc, lr

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

.text
.align	2
.global asm_enable_cache
asm_enable_cache:
  stmfd sp!, {r0, lr}
  mrc p15, 0, r0, c1, c0, 0 @ cp15 reg 1
  @ mmu on, both cache on
  orr r0, r0, #0x1000 
  orr r0, r0, #0x5
  mcr p15, 0, r0, c1, c0
  ldmfd sp!, {r0, pc}
  
