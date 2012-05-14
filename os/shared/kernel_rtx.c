/*--------------------------------------------------------------------------
 *                      Kernel RTX Implementations
 *--------------------------------------------------------------------------
 */

#include "kernel_rtx.h"

#include "rtx_inc.h"
#include "rtx.h"
#include "dbug.h"
#include "../memory/memory.h"
#include "../memory_pool/MemoryBlock.h"
#include "process_priority.h"
#include "../initialization/ProcessQueue.h"
#include "../initialization/InitTable.h"
#include "../scheduler/scheduler.h"
#include "../message_passing/Mailbox.h"

/* Interprocess Communications */
int kernel_send_message (int receiver_ID, void * messageEnvelope)
{


 #ifdef _DEBUG
//  rtx_dbug_outs((CHAR *)"Send msg to pid: ");
#endif

 #ifdef _DEBUG
  //rtx_dbug_out_number((int)receiver_ID);
#endif

  // Atomic on
  pcb* receiverPCB = message_receiver((int)receiver_ID);
   #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"Send msg to pid: ");
#endif

 #ifdef _DEBUG
  //rtx_dbug_out_number(receiverPCB->pid);
#endif
  // If no process of receiver_ID exists.
  if (receiverPCB == NULL){
 #ifdef _DEBUG
   // rtx_dbug_outs((CHAR *)"WARNING: Message no receiver.\r\n");
#endif

    return 1;
  }
 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"Message receiver exists \r\n");
#endif

  if (receiver_ID != 42){
	((Mail*)messageEnvelope)->m_delayed = FALSE;
  }
	
  Mailbox_putMailIn(&(receiverPCB->mailbox), messageEnvelope);
 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"mail is in\r\n");
#endif


  if (receiverPCB->state == BLOCKED_ON_MESSAGE)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"Receiver is blocked on message. Add receiver to ready queue then process switch.\r\n");
#endif


    receiverPCB->state = READY;
    append_process(receiverPCB);

    //pcb* _pcb = get_running_process();
    // if receiver has greater priority
    //if (receiverPCB->priority < _pcb->priority){
 #ifdef _DEBUG
    //  //rtx_dbug_outs((CHAR *)"process switch\r\n");
#endif

    //  _pcb->state = READY;
    //  process_switch();
    //}
    process_switch_if_there_is_a_higher_priority_process();
  }

  // Atomic off
  return 0;
}

void* kernel_receive_message(int* sender_ID)
{
  pcb* _pcb = get_running_process();
  
  //rtx_dbug_outs((CHAR *)"####################current_running_process_pid: ");
  //rtx_dbug_out_number(_pcb->pid);
  
  void* message = Mailbox_getMailOut(&(_pcb->mailbox), (UINT32*)sender_ID);

  while (message == NULL && _pcb->priority != I_PROCESS_PRIORITY)
  {
    // Change the process's state to be blocked on message.
    _pcb->state = BLOCKED_ON_MESSAGE;
 #ifdef _DEBUG
    //rtx_dbug_outs((CHAR *)"===========================================================\r\n");
#endif

 #ifdef _DEBUG
    //rtx_dbug_outs((CHAR *)"BLOCKED ON MESSAGE\r\n");
#endif

 #ifdef _DEBUG
    //rtx_dbug_outs((CHAR *)"===========================================================\r\n");
#endif


    process_switch();

    message = Mailbox_getMailOut(&(_pcb->mailbox), (UINT32*)sender_ID);
  }

  return message;
}

/*Memory Management*/
void* kernel_request_memory_block()
{
  void* userHeapPtr = NULL;

  // While there
  while (TRUE){
    userHeapPtr = s_request_memory_block();
	pcb* _pcb = get_running_process();
	
    if (userHeapPtr == NULL && _pcb->priority != I_PROCESS_PRIORITY)
    {
      // No memory available.
      // Set the running process's state from running->blocked
      
      // Set process state;
      _pcb->state = BLOCKED_ON_MEMORY;
 #ifdef _DEBUG
      rtx_dbug_outs((CHAR *)"===========================================================\r\n");
#endif

 #ifdef _DEBUG
      rtx_dbug_outs((CHAR *)"BLOCKED ON MEMORY\r\n");
#endif

 #ifdef _DEBUG
      rtx_dbug_outs((CHAR *)"===========================================================\r\n");
#endif


      process_switch();
    }
    else {
      // Found a valid block for process OR i-process. in which we cannot do anything.
      break;
    }
  }

  return userHeapPtr;
}

int kernel_release_memory_block(VOID* block)
{
    SINT32 rtnVal = s_release_memory_block(block);

    if (rtnVal == 0)
    {
      // Check if there are any processed blocked for memory;
	    pcb* blockedMemoryPCB = next_blocked_memory_process();
     if (blockedMemoryPCB  != NULL )
      {
        blockedMemoryPCB->state = READY;
        append_process(blockedMemoryPCB);

        //pcb* currentPCB = get_running_process();
        // if blocked's priority is higher (NOTE, lower is higher)
        //if (blockedMemoryPCB->priority < currentPCB->priority)
        //{
        //  process_switch();
        //}
        process_switch_if_there_is_a_higher_priority_process();
      }
    }

    return rtnVal;
}

/*Process Management*/
int kernel_release_processor()
{
    pcb* _pcb = get_running_process();
    _pcb->state = READY;
    process_switch();

    return 0;
}

/*Timing Service*/
int kernel_delayed_send(int receiverId, void* envelop, int delay)
{
  // Send message to timer-i process.

  pcb* runningPCB = get_running_process();
  pcb* receiverPCB = find_process((UINT32) receiverId);
  
  if(receiverPCB == NULL){
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"WARNING: delay_send Message no receiver.\r\n");
#endif

    return 1;
  }

  Mail* mail = (Mail*)envelop;
  mail->m_delaySenderID = runningPCB->pid;
  mail->m_destinationID = (UINT32)receiverId;
  mail->m_delayTime = delay;
  mail->m_delayed = TRUE;
  //rtx_dbug_outs((CHAR *)"WARNING: delay_send to ");
  //rtx_dbug_out_number(receiverId);
  //rtx_dbug_outs((CHAR *)"WARNING: delay_send to ");
  //rtx_dbug_out_number(mail->m_destinationID);

  // Send to i-process
  //Mailbox_putMailIn(&(timerPCB->mailbox), envelop);
  send_message(TIMER_I_PROCESS_PID, envelop);

  return 0;
}

/*Process Priority*/
int kernel_set_process_priority(int process_ID, int priority)
{
  if (process_ID == 0 || priority == LOWEST)
    return -1;
  
  set_priority_helper(process_ID, priority);

  process_switch_if_there_is_a_higher_priority_process();

  return 0;
}

int kernel_get_process_priority (int process_ID)
{
  pcb* cur = find_process((UINT32) process_ID);

  if (cur == NULL) {
    return -1;
  }
  
  return cur->priority;
}
