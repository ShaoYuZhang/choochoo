#include "../shared/rtx_inc.h"
#include "../shared/dbug.h"
#include "../shared/rtx.h"
#include "trap_handler.h"
#include "../shared/kernel_rtx.h"
#include "../atomic/atomic.h"

VOID c_trap_handler(SINT32 trapCode,
                    UINT32 arg1,
                    UINT32 arg2,
                    UINT32 arg3,
                    VOID* ptrToReturnVal)
{
  //atomic_on();
  if (trapCode == REQUEST_MEMORY_BLOCK_TRAP_CALL)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"REQUEST_MEMORY_BLOCK_TRAP_CALL\r\n");
#endif


    VOID** ptrToVoidPtr = (VOID**)ptrToReturnVal;
    *ptrToVoidPtr = kernel_request_memory_block();
  }
  else if (trapCode == RELEASE_MEMORY_BLOCK_TRAP_CALL)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"Release memory block trap call\r\n");
#endif


    SINT32* tmp = (SINT32*) ptrToReturnVal;
    *tmp = kernel_release_memory_block((void*)arg1);
  }
  else if (trapCode == RELEASE_PROCESSOR_TRAP_CALL)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"Release processor TRAP\r\n");
#endif


    SINT32* tmp = (SINT32*) ptrToReturnVal;
    *tmp = kernel_release_processor();
  }
  else if (trapCode == SEND_MESSAGE_TRAP_CALL)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"SEND MESSAGE TRAP\r\n");
#endif


    SINT32* rtn = (SINT32*)ptrToReturnVal;
    *rtn = kernel_send_message((int)arg1, (void*)arg2);
  }
  else if (trapCode == RECEIVE_MESSAGE_TRAP_CALL)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"Receive message trap call\r\n");
#endif


    VOID** ptrToVoidPtr = (VOID**)ptrToReturnVal;
    *ptrToVoidPtr = kernel_receive_message((int*)arg1);
  }
  else if (trapCode == DELAY_SEND_TRAP_CALL)
  {
#ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"Delay send trap call\r\n");
#endif
    SINT32* rtn = (SINT32*)ptrToReturnVal;
    *rtn = kernel_delayed_send((int)arg1, (void*)arg2, (int)arg3);
  }
  else if (trapCode == SET_PROCESS_PRIORITY_TRAP_CALL)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"SET process priority trap call\r\n");
#endif


    SINT32* rtn = (SINT32*)ptrToReturnVal;
    *rtn = kernel_set_process_priority((int)arg1, (int)arg2);
  }
  else if (trapCode == GET_PROCESS_PRIORITY_TRAP_CALL)
  {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"GET process priority trap call\r\n");
#endif

    SINT32* rtn = (SINT32*)ptrToReturnVal;
    *rtn = kernel_get_process_priority((int)arg1);
  }
  else {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR*)"WARNING: Unkown Trap Call code!!!\r\n");
#endif

  }
  //atomic_off();
}
