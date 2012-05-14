#ifndef kernel_rtx_h_
#define kernel_rtx_h_

int   kernel_send_message(int receiver_ID, void * messageEnvelope);
void* kernel_receive_message(int* sender_ID);
void* kernel_request_memory_block();
int   kernel_release_memory_block(void* block);
int   kernel_release_processor();
int   kernel_delayed_send(int process_ID, void* MessageEnvelope, int delay);
int   kernel_set_process_priority(int process_ID, int priority);
int   kernel_get_process_priority (int process_ID);

#endif // kernel_rtx_h_
