/*----------------------------------------------------------------------------
 *              A Dummy RTX for Testing
 *----------------------------------------------------------------------------
 */
/**
 * @file: main_rtx.c
 * @author: Thomas Reidemeister
 * @author: Irene Huang
 * @date: 2010.02.18
 * @brief: Example dummy rtx for testing
 */

#include "rtx.h"
#include "rtx_test.h"
#include "dbug.h"
#include "../memory/memory.h"
#include "../trap_handler/trap_handler.h"
#include "../initialization/InitTable.h"
#include "../initialization/SysProc.h"
#include "../initialization/UserProc.h"
#include "../memory_pool/MemoryPool.h"
#include "../initialization/Initialization.h"
#include "../scheduler/scheduler.h"
#include "memset.h"

/* test proc initializaiton info. registration function provided by test suite.
 * test suite needs to register its test proc initilization info with rtx
 * The __REGISTER_TEST_PROCS_ENTRY__ symbol is in linker scripts
 */

extern void __REGISTER_TEST_PROCS_ENTRY__();
extern char __end;
extern char* freeMemoryBegin;

user_proc_* create_user_table()
{
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Creating user procs init table.\r\n");
#endif

  user_proc_* u_cur, *u_tbl, *u_last;
  u_cur = NULL;
  u_tbl = register_user_process();
  u_last = u_tbl;
  while (u_last != NULL && u_last->next != NULL) {
    u_last = u_last->next;
  }
  
#ifdef _AUTOMATIC_TEST_
  int i = 0;
  for (; i < NUM_TEST_PROCS; i++) {
    u_cur = (user_proc_*) MemoryPool_alloc();
    u_cur->pid = g_test_proc[i].pid;
    u_cur->priority = g_test_proc[i].priority;
    u_cur->sz_stack = g_test_proc[i].sz_stack;
    u_cur->entry = g_test_proc[i].entry;
    //rtx_dbug_out_number((UINT32)u_cur->entry);

    u_cur->next = NULL;

    u_last->next = u_cur;
    u_last = u_cur;
  }
#endif  

 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"Finished user procs init table, entry:\r\n\r\n");
#endif


  return u_tbl;
}

int __main( void )
{
  return 0;
}

int main()
{
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"rtx: Entering main()\r\n");
#endif


  /* get the third party test proc initialization info */
  __REGISTER_TEST_PROCS_ENTRY__();

  /* The following  is just to demonstrate how to reference
   * the third party test process entry point inside rtx.
   * Your rtx should NOT call the test process directly!!!
   * Instead, the scheduler picks the test process to run
   * and the os context switches to the chosen test process
   */

   /* Load the vector table for TRAP #0 with our assembly stub
       address */
   asm( "move.l #asm_trap_entry,%d0" );
   asm( "move.l %d0,0x10000080" );

  //UINT32 turn = 100;
  //CALL_TRAP(SEND_MESSAGE_TRAP_CALL, 300,200,100, turn);

  //g_test_proc[0].entry(); /* DO NOT invoke test proc this way !!*/

  // Assign the start of free memory
  // (so we can start initializations that require memory.)
  freeMemoryBegin = &__end + 2; // Add two because __end is used.

  int diff = ((UINT32)freeMemoryBegin)%4;
  if (diff != 0){
    freeMemoryBegin  += (4-diff);
  }

  rtx_memset(freeMemoryBegin, 0, (char*)0x10200000 - freeMemoryBegin);

  init_user_heap();
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Created user heap.\r\n");
#endif


  MemoryPool_init();
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Created memory pool.\r\n");
#endif


  initialize(register_sys_proc(), create_user_table());
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Initialization finished.\r\n");
#endif


          int b;
        asm ("move.w %%sr, %0;":"=r"(b)  );    
rtx_dbug_out_number(b);		

  //UINT32 mask;
     /*
     * Set the interupt mask
     */
    //mask = SIM_IMR;
    //mask &= 0x0003ddff;
    //SIM_IMR = mask;
  process_switch();


  return 0;
}


/* register rtx primitives with test suite */

void  __attribute__ ((section ("__REGISTER_RTX__"))) register_rtx()
{
 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"rtx: Entering register_rtx()\r\n");
#endif

  g_test_fixture.send_message = send_message;
  g_test_fixture.receive_message = receive_message;
  g_test_fixture.request_memory_block = request_memory_block;
  g_test_fixture.release_memory_block = release_memory_block;
  g_test_fixture.release_processor = release_processor;
  g_test_fixture.delayed_send = delayed_send;
  g_test_fixture.set_process_priority = set_process_priority;
  g_test_fixture.get_process_priority = get_process_priority;
 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"rtx: leaving register_rtx()\r\n");
#endif

}
