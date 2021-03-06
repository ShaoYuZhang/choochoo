#ifndef INTERRUPT_H_
#define INTERRUPT_H_

void asm_handle_swi();
void asm_handle_hwi();
int asm_switch_to_usermode(int **sp_pointer);
int asm_syscall(int reqid, int a1, int a2, int a3);
void asm_enable_cache();

#endif // INTERRUPT_H_
