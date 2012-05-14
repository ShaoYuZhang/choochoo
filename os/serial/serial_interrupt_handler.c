#include "../shared/rtx_inc.h"
#include "../shared/rtx.h"
#include "../shared/dbug.h"
#include "../atomic/atomic.h"
#include "../scheduler/scheduler.h"
#include "../initialization/ProcessQueue.h"

VOID c_serial_handler(VOID)
{
  //atomic_on();
 //rtx_dbug_outs((CHAR *)"IN SERIAL current running process: ");
  //rtx_dbug_out_number(get_running_process()->pid);
  //rtx_dbug_outs((CHAR *)"current caller: ");
  //rtx_dbug_out_number(get_running_process()->pid);
  context_switch(get_uart_pcb());
  process_switch_if_there_is_a_higher_priority_process(); 

  //atomic_off();
}
