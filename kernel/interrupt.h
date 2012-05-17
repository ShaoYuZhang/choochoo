#ifndef INTERRUPT_H_
#define INTERRUPT_H_

void asm_handle_swi();
int  asm_syscall(int reqid, void** args);
void asm_switch_to_usermode(volatile register_set *reg);

#endif // INTERRUPT_H_
