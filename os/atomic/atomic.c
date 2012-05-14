#include "atomic.h"
#include "../scheduler/scheduler.h"

volatile SINT32 atomicCounter = 0;

void atomic_on(){
  //disable all interrupts
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *) "disabling all interrupts...\r\n");
#endif

  asm( "move.w #0x2700,%sr" );
  get_running_process()->atomicCounter++;
}

// Assume atomic_on was called.
void atomic_off(){
  get_running_process()->atomicCounter--;
  if(get_running_process()->atomicCounter == 0){
    //enable all interrupts
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *) "enabling all interrupts...\r\n");
#endif

    asm( "move.w #0x2000,%sr" );
  }
}



