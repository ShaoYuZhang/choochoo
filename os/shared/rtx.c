/*--------------------------------------------------------------------------
 *                      RTX Stub
 *--------------------------------------------------------------------------
 */
/**
 * @file:   rtx.c
 * @author: Irene Huang
 * @date:   2010.05.15
 * @brief:  Dummy rtx primitive stub implementation
 */

#include "rtx_inc.h"
#include "rtx.h"
#include "dbug.h"
#include "../trap_handler/trap_handler.h"

volatile char* freeMemoryBegin;// = NULL;

// Return a pointer to start of block of num_bytes size.
void* request_free_memory(UINT32 num_bytes)
{
  if(num_bytes <= 0){
    return NULL;
  }

 #ifdef _DEBUG
  rtx_dbug_outs((CHAR*)"Request_free_memory\r\n");
#endif

 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR*)"Allocating ");
#endif

 #ifdef _DEBUG
  //rtx_dbug_out_number(num_bytes);
#endif

 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR*)"Free memory before allocation:");
#endif

 #ifdef _DEBUG
  //rtx_dbug_out_number((UINT32)freeMemoryBegin);
#endif


  // Round number of bytes to the smallest factor of 4.
  int diff = num_bytes%4;
  if (diff != 0) num_bytes += (4 - diff);

  void* tmp = (void*)freeMemoryBegin;
  freeMemoryBegin += num_bytes;

 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR*)"Free memory  after allocation:");
#endif

 #ifdef _DEBUG
  //rtx_dbug_out_number((UINT32)freeMemoryBegin);
#endif


  return tmp;
}

/* Interprocess Communications*/
int send_message (int receiver_ID, void* messageEnvelope)
{
    UINT32 returnCode;
    CALL_TRAP(SEND_MESSAGE_TRAP_CALL, receiver_ID, messageEnvelope, NULL, returnCode);

    return (int)returnCode;
}

void * receive_message(int* sender_ID)
{
    UINT32 returnCode;
    CALL_TRAP(RECEIVE_MESSAGE_TRAP_CALL, sender_ID, NULL, NULL, returnCode);

    return (void*)returnCode;
}

/*Memory Management*/
void * request_memory_block()
{
    UINT32 returnCode;
    CALL_TRAP(REQUEST_MEMORY_BLOCK_TRAP_CALL, NULL, NULL, NULL, returnCode);

    return (void*)returnCode;
}

int release_memory_block(VOID* block)
{
    UINT32 returnCode;
    CALL_TRAP(RELEASE_MEMORY_BLOCK_TRAP_CALL, block, NULL, NULL, returnCode);

    return (int)returnCode;
}

/*Process Management*/
int release_processor()
{
    UINT32 returnCode;
    CALL_TRAP(RELEASE_PROCESSOR_TRAP_CALL,1, 2, 3, returnCode);

    return (int)returnCode;
}

/*Timing Service*/
int delayed_send(int process_ID, void* MessageEnvelope, int delay)
{
    UINT32 returnCode;
    CALL_TRAP(DELAY_SEND_TRAP_CALL, process_ID, MessageEnvelope, delay, returnCode);
    return returnCode;
}

/*Process Priority*/
int set_process_priority (int process_ID, int priority)
{
    //CALL_TRAP(SET_PROCESS_PRIORITY_TRAP_CALL);
    return 0;
}

int get_process_priority (int process_ID)
{
    //CALL_TRAP(GET_PROCESS_PRIORITY_TRAP_CALL);
    return 0;
}
