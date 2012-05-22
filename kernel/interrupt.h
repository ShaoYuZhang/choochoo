#ifndef INTERRUPT_H_
#define INTERRUPT_H_

void asm_handle_swi();
void asm_switch_to_usermode(int **sp_pointer);
void asm_memcpy_no_overlap(char* from, char* to, int len);

#endif // INTERRUPT_H_
