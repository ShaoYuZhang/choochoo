// Header file for process queue

#ifndef _PROCESS_QUEUE_H_
#define _PROCESS_QUEUE_H_

#include "../shared/rtx_inc.h"
#include "../shared/process_priority.h"
#include "../message_passing/Mailbox.h"

enum Pcb_State
{ READY,
  BLOCKED_ON_MEMORY,
  BLOCKED_ON_MESSAGE,
  RUNNING,
  INTERRUPTED
};

typedef enum Pcb_State Pcb_State;

typedef struct pcb {
  UINT32 pid;
  UINT32 atomicCounter;
  VOID* stack;
  enum Process_Priority priority;
  enum Pcb_State state;
  Mailbox mailbox;
  struct pcb *previousProcess;

  struct pcb* next;
} pcb;

typedef struct p_queue{
  pcb* begin;
  pcb* end;
} p_queue;

pcb* pcb_new(UINT32, enum Process_Priority, UINT32, VOID*, BOOLEAN, BOOLEAN);

// Direct access to process queue on different levels

VOID append_process(pcb* process);

//pcb* pull_process(UINT32 priority);

void set_priority_helper(UINT32 pid, Process_Priority priority);

pcb* next_ready_process();

pcb* get_timer_pcb();

pcb* get_uart_pcb();

Process_Priority next_priority();

pcb* next_blocked_memory_process();

VOID append_message_blocked(pcb* process);

pcb* message_receiver(UINT32 pid);

VOID init_p_queue();

VOID dump_queue();

VOID dump_pcb(pcb*);

VOID dump_all();

pcb* find_process(UINT32 pid);

#endif /* _PROCESS_QUEUE_H_ */
