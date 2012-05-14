#include "UserProc.h"
#include "../shared/rtx.h"
#include "../shared/dbug.h"
#include "../memory_pool/MemoryPool.h"
#include "ProcessABC.h"
#include "../wall_clock/wall_clock_process.h"
#include "../set_process_priority_process/set_process_priority_process.h"

user_proc_* register_user_process()
{
  user_proc_*	u_cur, *u_last, *u_tbl;

  // Add process A, B, C
  u_cur	= (user_proc_*) MemoryPool_alloc();
  u_cur->pid = PROCESS_A_PID;
  u_cur->priority   = 1;
  u_cur->sz_stack   = 2048;
  u_cur->entry      = user_process_a;
  u_cur->next	      = NULL;

  u_tbl = u_cur;
  u_last = u_cur;
  
  u_cur = (user_proc_*) MemoryPool_alloc();
  u_cur->pid = PROCESS_B_PID;
  u_cur->priority = 1;
  u_cur->sz_stack = 2048;
  u_cur->entry = user_process_b;
  u_cur->next = NULL;

  u_last->next = u_cur;
  u_last = u_cur;

  u_cur = (user_proc_*) MemoryPool_alloc();
  u_cur->pid = PROCESS_C_PID;
  u_cur->priority = 1;
  u_cur->sz_stack = 2048;
  u_cur->entry = user_process_c;
  u_cur->next = NULL;

  u_last->next = u_cur;
  u_last = u_cur;

  // Add Wall Clock process
  u_cur = (user_proc_*) MemoryPool_alloc();
  u_cur->pid = WALL_CLOCK_PID;
  u_cur->priority = 0;
  u_cur->sz_stack = 2048;
  u_cur->entry = wall_clock;
  u_cur->next = NULL;

  u_last->next = u_cur;
  u_last = u_cur;
  
  // Add set process priority process
  u_cur = (user_proc_*) MemoryPool_alloc();
  u_cur->pid = SET_PROCESS_PRIORITY_PROCESS_PID;
  u_cur->priority = 0;
  u_cur->sz_stack = 2048;
  u_cur->entry = set_process_priority_process;
   u_cur->next = NULL;

  u_last->next = u_cur;
  u_last = u_cur;


  return u_tbl;
}
