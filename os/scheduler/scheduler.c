#include "scheduler.h"
#include "../shared/dbug.h"
#include "../initialization/ProcessQueue.h"

pcb *currentRunningProcess = NULL;

VOID process_switch(VOID)
{
  //dump_queue();
  if (currentRunningProcess != NULL)
  {
    append_process(currentRunningProcess);
  }
  pcb *nextRunning = next_ready_process();
  context_switch(nextRunning);

}

VOID context_switch_back_from_i_process()
{
  //rtx_dbug_outs((CHAR *)"current running's previous: ");
  //rtx_dbug_out_number(get_running_process()->previousProcess->pid);
  context_switch(currentRunningProcess-> previousProcess);
}


VOID process_switch_if_there_is_a_higher_priority_process()
{
  if ((SINT32)next_priority() < (SINT32)currentRunningProcess->priority)
  {
    currentRunningProcess->state = READY;
    process_switch();
  }
}

pcb* get_running_process(){
  return currentRunningProcess;
}

VOID context_switch_helper(VOID *oldStackPointer, pcb* nextRunning)
{

 #ifdef _DEBUG
    rtx_dbug_out_number(nextRunning);
#endif

  if (currentRunningProcess != NULL)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"if current process is not null hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\r\n");
#endif


 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"old stack pointer hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\r\n");
#endif

 #ifdef _DEBUG
  rtx_dbug_out_number(oldStackPointer);
#endif

 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"current stack is  hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\r\n");
#endif

 #ifdef _DEBUG
  rtx_dbug_out_number(currentRunningProcess->stack);
#endif

    currentRunningProcess->stack = oldStackPointer;
  }
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)" before change \r\n");
#endif


 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"old stack pointer \r\n");
#endif

 #ifdef _DEBUG
  rtx_dbug_out_number(oldStackPointer);
#endif

 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"current stack is \r\n");
#endif

 #ifdef _DEBUG
  rtx_dbug_out_number(currentRunningProcess->stack);
#endif


  nextRunning->previousProcess = currentRunningProcess;
  currentRunningProcess = nextRunning;
  currentRunningProcess->state = RUNNING;
  oldStackPointer = currentRunningProcess->stack;
 
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"PID of new Process\r\n");
#endif
 
 #ifdef _DEBUG
 rtx_dbug_out_number(currentRunningProcess->pid);
#endif


 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)" after change\r\n");
#endif


 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"old stack pointer\r\n");
#endif

 #ifdef _DEBUG
  rtx_dbug_out_number(oldStackPointer);
#endif

 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"current stack is\r\n");
#endif

 #ifdef _DEBUG
  rtx_dbug_out_number(currentRunningProcess->stack);
#endif

}
