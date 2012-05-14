#ifndef _INIT_TABLE_H_
#define _INIT_TABLE_H_

#include "../shared/process_priority.h"
#include "../shared/rtx_inc.h"

#define DEFAULT_SR_USER 	0x40800000
#define DEFAULT_SR_SYS  	0x40802000
#define DEFAULT_SR_I_PROCESS	0x40802700
#define TIMER_I_PROCESS_PID	42
#define UART_I_PROCESS_PID	66

typedef struct user_proc_{
  UINT32  pid;           /* pid of a test process */
  Process_Priority  priority;      /* initial priority of a test process */
  UINT32 sz_stack;      /* stack size of a test process */
  VOID   (*entry)();    /* entry point of a test process */
  struct user_proc_* next;
} user_proc_;

typedef struct sys_proc_{
  UINT32  pid;           /* pid of a test process */
  Process_Priority  priority;      /* initial priority of a test process */
  UINT32 sz_stack;      /* stack size of a test process */
  VOID   (*entry)();    /* entry point of a test process */
  BOOLEAN i_process;
  struct sys_proc_* next;
} sys_proc_;

#endif /* _INIT_TABLE_H_ */
