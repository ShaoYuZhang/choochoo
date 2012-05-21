#ifndef INTERRUPT_H_
#define INTERRUPT_H_

void asm_handle_swi();
void asm_switch_to_usermode(volatile register_set *reg);

#endif // INTERRUPT_H_
