#include "../shared/dbug.h"
#include "../shared/rtx.h"
#include "SysProc.h"
#include "InitTable.h"
#include "../shared/process_priority.h"
#include "../memory_pool/MemoryPool.h"
#include "../keyboard_decoder_process/keyboard_decoder_process.h"
#include "../crt_display/crt_display.h"
#include "../serial/serial.h"
#include "../timer/timer0.h"

void null_process()
{
  while (1)
    {
 #ifdef _DEBUG
      rtx_dbug_outs((CHAR *)"sys process: null process\r\n");
#endif

      release_processor();
    }
}

sys_proc_* register_sys_proc()
{
#ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Create sys process init table.\r\n");
#endif


  //create sys table and return the pointer
  sys_proc_*	s_cur, *s_tbl, *s_last;
  s_cur	            = (sys_proc_*) MemoryPool_alloc();
  s_cur->pid	      = 0;
  s_cur->priority   = LOWEST;
  s_cur->sz_stack   = 2048;
  s_cur->entry      = null_process;
  s_cur->i_process = FALSE;
  s_cur->next	      = NULL;

  s_tbl = s_cur;
  s_last = s_cur;
  
  // Initialize KCD process table
  s_cur = (sys_proc_*) MemoryPool_alloc();
  s_cur->pid = KEYBOARD_DECODER_PROCESS_PID;
  s_cur->priority = 0;
  s_cur->sz_stack = 2048;
  s_cur->entry = keyboard_decoder_process;
  s_cur->i_process = FALSE;
  s_cur->next = NULL;

  s_last->next = s_cur;
  s_last = s_cur;

  // Initialize CRT process table
  s_cur = (sys_proc_*) MemoryPool_alloc();
  s_cur->pid = CRT_DISPLAY_PID;
  s_cur->priority = 1;
  s_cur->sz_stack = 2048;
  s_cur->entry = crt_display_process;
  s_cur->i_process = FALSE;
  s_cur->next = NULL;

  s_last->next = s_cur;
  s_last = s_cur;
  
  s_cur = (sys_proc_*) MemoryPool_alloc();
  s_cur->pid = TIMER_I_PROCESS_PID;
  s_cur->priority = I_PROCESS_PRIORITY;
  s_cur->sz_stack = 2048;
  s_cur->entry = timer_i_process;
  s_cur->i_process = TRUE;
  s_cur->next = NULL;

  s_last->next = s_cur;
  s_last = s_cur;

  s_cur = (sys_proc_*) MemoryPool_alloc();
  s_cur->pid = UART_I_PROCESS_PID;
  s_cur->priority = I_PROCESS_PRIORITY;
  s_cur->sz_stack = 2048;
  s_cur->entry = uart_i_process;
  s_cur->i_process = TRUE;
  s_cur->next = NULL;

  s_last->next = s_cur;
  s_last = s_cur;

  return s_tbl;
}
