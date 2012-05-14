#include "ProcessQueue.h"
#include "../memory_pool/MemoryPool.h"
#include "../shared/rtx.h"
#include "../shared/dbug.h"
#include "../shared/process_priority.h"
#include "InitTable.h"
#include "../scheduler/scheduler.h"

p_queue priorityQueues[NUM_OF_ALL_QUEUES_USED_BY_SYSTEM];

// ---------------------------------------------------------------------------
pcb* pcb_new(UINT32 pid_,
             enum Process_Priority priority_,
             UINT32 stack_size,
             VOID* pc_,
             BOOLEAN user_mode,
             BOOLEAN i_process)
{
  // Use request to request stack memory
  VOID* stack_ = request_free_memory(stack_size);

  pcb* current = (pcb*) MemoryPool_alloc();
 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"Create new PCB, number:");
  //rtx_dbug_out_number(pid_);
  //rtx_dbug_outs((CHAR *)"\r\n");
#endif


  current->pid = pid_;
  current->priority = priority_;

 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"    priority:");
  //rtx_dbug_out_number(priority_);
  //rtx_dbug_outs((CHAR *)"\r\n");
#endif

  current->state = READY;

  // |            | <-- stack_ (Top of stack, given by request_free_memory)
  // |            |
  // |            |
  // |            |
  // |            |
  // |            |
  // |            | <-- stack_ + stack_size - 12
  // | DEFAULT_SR |
  // |_pc_________| <-- stack_ + stack_size
  char* st = (char*) stack_;

 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"    stack address:");
  //rtx_dbug_out_number((UINT32)stack_);
  //rtx_dbug_outs((CHAR *)"\r\n");
#endif


  st = st + stack_size - 4;
  UINT32* temp = (UINT32*) st;
  *temp = (UINT32)pc_;
  st = st - 4;
  temp = (UINT32*) st;
  //Determine in the fake stack which SR value to use
  *temp = user_mode ? DEFAULT_SR_USER
                    : (i_process ? DEFAULT_SR_I_PROCESS
                                 : DEFAULT_SR_SYS);
  st = st - 4;
  temp = (UINT32*) st;
  asm( "move.l #trap_handler_exit, %0;" : "=r"(*temp));

  st = st - 4;
  temp = (UINT32*) st;
  *temp = 0;

  st = st - 4;
  temp = (UINT32*) st;
  *temp = 0;

  st = st - 4;
  temp = (UINT32*) st;
  *temp = 0x2700; //fake sr

  st = st - 4;
  temp = (UINT32*) st;
  *temp = 0;
  //st = st -4;
  stack_ = (VOID*) st;
  current->stack = stack_;
  current->atomicCounter = 0;
#ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"    pc:");
  //rtx_dbug_out_number((UINT32)pc_);
  //rtx_dbug_outs((CHAR *)"\r\n");
#endif
  current->previousProcess = NULL;
  current->next = NULL;

  Mailbox_construct(&(current->mailbox));

  return current;
}

// ---------------------------------------------------------------------------
VOID init_p_queue() {
  int i = 0;
  for (; i < NUM_OF_ALL_QUEUES_USED_BY_SYSTEM; i++) {
    priorityQueues[i].begin = NULL;
    priorityQueues[i].end = NULL;
  }
}

// ---------------------------------------------------------------------------
// Direct access to process queue on different levels
VOID append_process(pcb* process)
{
  int q_priority;
  switch (process->state)
  {
    case BLOCKED_ON_MEMORY:
    {
      q_priority = BLOCKED_MEMORY;
      break;
    }
    case BLOCKED_ON_MESSAGE:
    {
      q_priority = BLOCKED_MESSAGE;
      break;
    }
    case READY:
    default:
    {
      q_priority = process->priority;
      break;
    }
  }
  if (process->priority == I_PROCESS_PRIORITY) {
	// Ignore i process because they are always in the queue
	//return;
	q_priority = I_PROCESS_QUEUE;
#ifdef _DEBUG
	rtx_dbug_outs((CHAR *)"Append_process Pid as i process:");
	rtx_dbug_out_number(q_priority);
	rtx_dbug_out_number(process->pid);
#endif
  }
    
  process->next = NULL;

#ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Append_process Priority:");
  rtx_dbug_out_number(q_priority);
  rtx_dbug_out_number(process->pid);
#endif


  if (priorityQueues[q_priority].begin == NULL) {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"Empty\r\n");
#endif

    priorityQueues[q_priority].begin = process;
    priorityQueues[q_priority].end = process;
  } else {
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"To end\r\n");
	dump_pcb(priorityQueues[q_priority].end);
#endif

    priorityQueues[q_priority].end->next = process;
    priorityQueues[q_priority].end = process;
  }
}

// ---------------------------------------------------------------------------
pcb* pull_process(Queue_Priority q_priority) {
  //Cannot pull out i=process
  if (q_priority == I_PROCESS_PRIORITY)
	return NULL;
  pcb * result = priorityQueues[q_priority].begin;
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Pull process pid: ");
  rtx_dbug_out_number((int)result->pid);
  rtx_dbug_outs((CHAR *)"\r\n");
#endif

  if (priorityQueues[q_priority].begin != NULL) {
    priorityQueues[q_priority].begin = priorityQueues[q_priority].begin->next;
	if (priorityQueues[q_priority].begin == NULL) {
		priorityQueues[q_priority].end = NULL;
	}
  } else {
    return NULL;
  }
  result->next = NULL;
  return result;
}

// ---------------------------------------------------------------------------
pcb* find_pcb_from_single_queue(Queue_Priority q_priority, UINT32 pid) {
  pcb* cur = priorityQueues[q_priority].begin;
  while (cur != NULL) {
    if (cur->pid == pid) {
      return cur;
    }
    cur  = cur->next;
  }
  return NULL;
}

// ---------------------------------------------------------------------------
pcb* grab_pcb_from_single_queue(Queue_Priority q_priority, UINT32 pid) {
  //Cannot pull out i=process
  if (q_priority == I_PROCESS_PRIORITY)
	return NULL;
  p_queue* queue = &(priorityQueues[q_priority]);
  pcb* cur = queue->begin;
  if (cur == NULL){
    return cur;
  }

  // Delete from begin
  if (cur->pid == pid) {
    queue->begin = cur->next;
    if (queue->begin == NULL)
      queue->end = NULL;
    cur->next = NULL;
    return cur;
  }

  // Delete from middle.
  pcb * prev = cur;
  cur = cur->next;
  while (cur != NULL)
  {
    if (cur->pid == pid) {
      prev->next = cur->next;
      if (queue->end == cur) {
		queue->end = prev;
	  }
      cur->next = NULL;
      return cur;
    }
    prev = cur;
    cur = cur->next;
  }

  return NULL;
}

// ---------------------------------------------------------------------------
pcb* find_process(UINT32 pid){
  Queue_Priority i = HIGHEST;

  while (i < NUM_OF_QUEUES) {
    pcb* cur = find_pcb_from_single_queue(i, pid);
    if (cur != NULL) {
      return cur;
    }
    i++;
  }

  pcb* cur = get_running_process();

  if (cur != NULL && cur->pid == pid) {
    return cur;
  }

  if (cur->previousProcess != NULL && cur->previousProcess->pid == pid) {
    return cur->previousProcess;
  }
  return NULL;
}

// ---------------------------------------------------------------------------
pcb* next_ready_process() {
  int i = 0;
  while (i < NUM_OF_PRIORITIES) {
    if (priorityQueues[i].begin != NULL)
      return pull_process(i);
    i++;
  }
  return NULL;
}

// ---------------------------------------------------------------------------
Process_Priority next_priority() {
  Process_Priority i = HIGHEST;
  while (i < NUM_OF_PRIORITIES) {
    if (priorityQueues[i].begin != NULL)
      return i;
    i++;
  }
  return LOWEST; // null process's priority
}

// ---------------------------------------------------------------------------
VOID dump_queue() {
//#ifdef _DEBUG
 //#ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Dump all queues: \r\n");
//#endif

  int i = 0;
  pcb* p_iter;
  while (i < /*NUM_OF_ALL_QUEUES_USED_BY_SYSTEM){//*/NUM_OF_QUEUES) {
 //#ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"  priority: ");
    rtx_dbug_out_number((int)i);
//#endif


    p_iter = priorityQueues[i].begin;
    while (p_iter != NULL) {
 //#ifdef _DEBUG
      rtx_dbug_outs((CHAR *)"    pid: ");
      rtx_dbug_out_number((int)p_iter->pid);
//#endif


      p_iter = p_iter->next;
    }
    i++;
  }
  p_iter = get_running_process();
 //#ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"Current running process pid: ");
  rtx_dbug_out_number((int)p_iter->pid);
//#endif

//#endif
}

//---------------------------------------------------------------------------
VOID dump_pcb(pcb* cur) {
 //#ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"    pid: ");
  rtx_dbug_out_number((int)cur->pid);
  //rtx_dbug_outs((CHAR *)"    stack: ");
  //rtx_dbug_out_number((int)cur->stack);
  rtx_dbug_outs((CHAR *)"    piority: ");
    rtx_dbug_out_number((int)cur->priority);
    rtx_dbug_outs((CHAR *)"    state: ");
    rtx_dbug_out_number((int)cur->state);
//#endif
}

// ---------------------------------------------------------------------------
VOID dump_all() {
//#ifdef _DEBUG_HOTKEY
  rtx_dbug_outs((CHAR *)"Display processes on ready queues: \r\n");

  int i = HIGHEST;
  pcb* p_iter;
  while (i < NUM_OF_QUEUES) {
    p_iter = priorityQueues[i].begin;
    while (p_iter != NULL) {
      rtx_dbug_outs((CHAR *)"  pid: ");
      rtx_dbug_out_number((int)p_iter->pid);
      rtx_dbug_outs((CHAR *)"\r\n");
      rtx_dbug_outs((CHAR *)"    priority: ");
      rtx_dbug_out_number((int)p_iter->priority);
      rtx_dbug_outs((CHAR *)"\r\n");

      p_iter = p_iter->next;
    }
    i++;
  }
  i = BLOCKED_MEMORY;
  rtx_dbug_outs((CHAR *)"Processes blocked on memory: \r\n");

  p_iter = priorityQueues[i].begin;
  while (p_iter != NULL) {
    rtx_dbug_outs((CHAR *)"  pid: ");
    rtx_dbug_out_number((int)p_iter->pid);
    rtx_dbug_outs((CHAR *)"\r\n");
    rtx_dbug_outs((CHAR *)"    priority: ");

    rtx_dbug_out_number((int)p_iter->priority);
    rtx_dbug_outs((CHAR *)"\r\n");

    p_iter = p_iter->next;
  }
//#endif
}

// ---------------------------------------------------------------------------
pcb* next_blocked_memory_process() {
  pcb* cur;
  Queue_Priority i = BLOCKED_MEMORY;
  Process_Priority priority = LOWEST;

  // Find highest priority
  cur = priorityQueues[i].begin;
  while (cur != NULL) {
    if (cur->priority < priority)
      priority = cur->priority;
    cur = cur->next;
  }

 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"Highest priority found: ");
  //rtx_dbug_out_number((int)priority);
  //rtx_dbug_outs((CHAR *)"\r\n");
#endif


  cur = priorityQueues[i].begin;
  if (cur == NULL){
    return cur;
  }
 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"pcb priority is: ");
  //rtx_dbug_out_number((int)cur->priority);
  //rtx_dbug_outs((CHAR *)" while pid is ");
  //rtx_dbug_out_number((int)cur->pid);
  //rtx_dbug_outs((CHAR *)"\r\n");
#endif

  if (cur->priority == priority) {
    priorityQueues[i].begin = cur->next;
    if (priorityQueues[i].begin == NULL)
      priorityQueues[i].end = NULL;
    cur->next = NULL;
    return cur;
  }
  pcb* prev = priorityQueues[i].begin;
  prev = cur;
  cur = cur->next;
  while (cur != NULL) {
    if (cur->priority == priority) {

 #ifdef _DEBUG
  //rtx_dbug_outs((CHAR *)"pcb priority is: ");
  //rtx_dbug_out_number((int)cur->priority);
  //rtx_dbug_outs((CHAR *)" while pid is ");
  //rtx_dbug_out_number((int)cur->pid);
  //rtx_dbug_outs((CHAR *)"\r\n");
#endif

      prev->next = cur->next;
      if (priorityQueues[i].end == cur)
        priorityQueues[i].end = prev;
      cur->next = NULL;
      return cur;
    }
    prev = cur;
    cur = cur->next;
  }

  return NULL;
}

// ---------------------------------------------------------------------------
pcb* message_receiver(UINT32 pid) {
  //dump_queue();
  int i = HIGHEST;
  pcb * cur;

  while (i < BLOCKED_MESSAGE) {
 #ifdef _DEBUG
    //rtx_dbug_outs((CHAR *)"Look for receiver in  priority: ");
    //rtx_dbug_out_number((int)i);
    //rtx_dbug_outs((CHAR *)"\r\n");
#endif

    cur = find_pcb_from_single_queue(i, pid);
    if (cur != NULL)
      return cur;
    i++;
  }
  
  cur = find_pcb_from_single_queue(I_PROCESS_QUEUE, pid);
  if (cur != NULL)
    return cur;
  
  cur = grab_pcb_from_single_queue(BLOCKED_MESSAGE, pid);
  if (cur != NULL)
    return cur;
  
    cur = get_running_process();
  if (cur != NULL && cur->pid == pid)
    return cur;
  if (cur->previousProcess != NULL && cur->previousProcess->pid == pid) {
    return cur->previousProcess;
  }
}

// ---------------------------------------------------------------------------
void set_priority_helper(UINT32 pid, Process_Priority priority) {
  int i = HIGHEST;
  pcb * cur;

  // If the process is current running
  // Then only need to set the priority
  cur = get_running_process();
  if (cur != NULL && cur->pid == pid) {
    cur->priority = priority;
    return;
  }
  if (cur->previousProcess != NULL && cur->previousProcess->pid == pid) {
    cur->previousProcess->priority = priority;;
    return; 
  }
  
  while (i < LOWEST) {
    cur = grab_pcb_from_single_queue(i, pid);
    if (cur != NULL) {
      cur->priority = priority;
      append_process(cur);
      return;
    }
    i++;
  }

  cur = find_pcb_from_single_queue(BLOCKED_MESSAGE, pid);
  if (cur != NULL) {
    cur->priority = priority;
  }

  cur = find_pcb_from_single_queue(BLOCKED_MEMORY, pid);
  if (cur != NULL) {
    cur->priority = priority;
  }
}

// ---------------------------------------------------------------------------
pcb* get_timer_pcb(){
  return find_pcb_from_single_queue(I_PROCESS_QUEUE, TIMER_I_PROCESS_PID);
}

// ---------------------------------------------------------------------------
pcb* get_uart_pcb(){
  return find_pcb_from_single_queue(I_PROCESS_QUEUE, UART_I_PROCESS_PID);
}
