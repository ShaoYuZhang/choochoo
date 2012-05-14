/*--------------------------------------------------------------------------
 *                      RTX Stub header
 *--------------------------------------------------------------------------
 */
/**
 * @file:   rtx.h
 * @author: Irene Huang
 * @date:   2010.05.15
 * @brief:  header file defining the rtx primitive prototype
 */

#ifndef _RTX_H_
#define _RTX_H_
#include "rtx_inc.h"
#include "process_priority.h"

void* request_free_memory(UINT32 num_bytes);

/* Primitives as specified in the project description*/
/* Interprocess Communications*/
int send_message (int receiver_ID, void * MessageEnvelope);
void * receive_message(int * sender_ID);

/*Memory Management*/
void * request_memory_block();
int release_memory_block(void * MemoryBlock);

/*Process Management*/
int release_processor();

/*Timing Service*/
int delayed_send(int process_ID, void * MessageEnvelope, int delay);

/*Process Priority*/
int set_process_priority (int process_ID, int priority);
int get_process_priority (int process_ID);

#endif /* _RTX_H_ */
