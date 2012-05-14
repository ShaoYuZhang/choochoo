#include "../shared/rtx_inc.h"
#include "../shared/rtx.h"
#include "../shared/dbug.h"
#include "../atomic/atomic.h"
#include "../scheduler/scheduler.h"

VOID c_timer_handler(VOID)
{

  //rtx_dbug_outs((CHAR *)"timer_handler");
  
  //atomic_on();
  //rtx_dbug_outs((CHAR *)"current running process: ");
  //rtx_dbug_out_number(get_running_process()->pid);
  //rtx_dbug_outs((CHAR *)"current caller: ");
  //rtx_dbug_out_number(get_running_process()->pid);
  pcb *temp = get_timer_pcb();
  //rtx_dbug_out_number(temp);
  context_switch(temp);
  process_switch_if_there_is_a_higher_priority_process(); 

  //atomic_off();
}
