#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <ts7200.h>
#include <task.h>

#define NUM_PRIORITY 10 // 0 = HIGHEST, 9 = LOWEST
#define MAX_PRIORITY (NUM_PRIORITY - 1)
#define MIN_PRIORITY 0

void scheduler_init();

volatile TaskDescriptor *scheduler_get_running();

void scheduler_append(volatile TaskDescriptor *td);

int scheduler_empty();

volatile TaskDescriptor* scheduler_get();

void scheduler_killme();

void scheduler_move2ready();

#endif // SCHEDULER_H_
