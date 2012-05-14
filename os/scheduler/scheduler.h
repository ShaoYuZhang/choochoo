#ifndef scheduler_h_
#define scheduler_h_

#include "../initialization/ProcessQueue.h"

pcb* get_running_process();

VOID process_switch();

VOID context_switch(pcb* nextRunning);

VOID process_switch_if_there_is_a_higher_priority_process();

VOID context_switch_back_from_i_process();
#endif
