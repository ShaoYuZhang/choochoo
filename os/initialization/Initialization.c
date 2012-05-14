#include "Initialization.h"
#include "../shared/rtx_inc.h"
#include "../shared/dbug.h"
#include "../shared/process_priority.h"
#include "ProcessQueue.h"
#include "InitTable.h"

extern pcb* currentRunningProcess;

SINT32 coldfire_vbr_init( VOID )
{
    /*
     * Move the VBR into real memory
     *
     * DG: actually, it'll already be here.
     */
    asm( "move.l %a0, -(%a7)" );
    asm( "move.l #0x10000000, %a0 " );
    asm( "movec.l %a0, %vbr" );
    asm( "move.l (%a7)+, %a0" );
    
    return RTX_SUCCESS;
}

void initalize_i_process_asm() {
  coldfire_vbr_init();

  {	
    UINT32 mask;

    /*
     * Store the timer ISR at auto-vector #6
     */
    asm( "move.l #asm_timer_entry,%d0" );
    asm( "move.l %d0,0x10000078" );

    /*
     * Setup to use auto-vectored interupt level 6, priority 3
     */
    TIMER0_ICR = 0x9B;

    /*
     * Set the reference counts, ~10ms
     */
    TIMER0_TRR = 176; 

    /*
     * Setup the timer prescaler and stuff
     */
    TIMER0_TMR = 0xFF1B;

    /*
     * Set the interupt mask
     */
    mask = SIM_IMR;
    mask &= 0x0003fdff;
    SIM_IMR = mask;    
  }

  {
    UINT32 mask;
        
    /*
     * Store the serial ISR at user vector #64
     */
    asm( "move.l #asm_serial_entry,%d0" );
    asm( "move.l %d0,0x10000100" );

    /* Reset the entire UART */
    SERIAL1_UCR = 0x10;

    /* Reset the receiver */
    SERIAL1_UCR = 0x20;
    
    /* Reset the transmitter */
    SERIAL1_UCR = 0x30;

    /* Reset the error condition */
    SERIAL1_UCR = 0x40;

    /* Install the interupt */
    SERIAL1_ICR = 0x17;
    SERIAL1_IVR = 64;

    /* enable interrupts on rx only */
    SERIAL1_IMR = 0x02;

    /* Set the baud rate */
    SERIAL1_UBG1 = 0x00;
#ifdef _CFSERVER_           /* add -D_CFSERVER_ for cf-server build */
    SERIAL1_UBG2 = 0x49;    /* cf-server baud rate 19200 */ 
#else
    SERIAL1_UBG2 = 0x92;    /* lab board baud rate 9600 */
#endif /* _CFSERVER_ */

    /* Set clock mode */
    SERIAL1_UCSR = 0xDD;

    /* Setup the UART (no parity, 8 bits ) */
    SERIAL1_UMR = 0x13;
    
    /* Setup the rest of the UART (noecho, 1 stop bit ) */
    SERIAL1_UMR = 0x07;

    /* Setup for transmit and receive */
    SERIAL1_UCR = 0x05;

    /* Enable interupts */
    mask = SIM_IMR;
    mask &= 0x0003dfff;
    SIM_IMR = mask;
  }
}

void initialize(sys_proc_* sys_tbl, user_proc_* user_tbl) {
  currentRunningProcess = NULL;
  // Create p_queue
  init_p_queue();

  // Read from table
  // First read from test process

  // Then read from system process (Not needed for P2)

  // Create pcb
  pcb * new_cur;
  user_proc_* u_cur = user_tbl;
  sys_proc_* s_cur = sys_tbl;
  user_proc_* u_temp;
  sys_proc_* s_temp;

  while (u_cur != NULL) {
    new_cur = pcb_new(u_cur->pid, u_cur->priority, u_cur->sz_stack, u_cur->entry, TRUE, FALSE);
    append_process(new_cur);
    u_temp = u_cur;
    u_cur = u_cur->next;
    MemoryPool_free((VOID*) u_temp);
  }

  while (s_cur != NULL) {
    new_cur = pcb_new(s_cur->pid, s_cur->priority, s_cur->sz_stack, s_cur->entry, FALSE, s_cur->i_process);
    append_process(new_cur);    
    s_temp = s_cur;
    s_cur = s_cur->next;
    MemoryPool_free((VOID*) s_temp);
  }
  
  initalize_i_process_asm();
}
