#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <ts7200.h>
#include <task.h>

#define NUM_PRIORITY 31 // 0 = HIGHEST, 30 = LOWEST
#define MAX_PRIORITY (NUM_PRIORITY - 1)
#define MIN_PRIORITY 0

void scheduler_init();

volatile TaskDescriptor* scheduler_get_running();

void scheduler_append(volatile TaskDescriptor *td);

int scheduler_empty();

volatile TaskDescriptor* scheduler_get();

void scheduler_set_running(volatile TaskDescriptor* td);

void scheduler_move2ready();

#endif // SCHEDULER_H_
