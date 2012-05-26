#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <ts7200.h>
#include <task.h>

void scheduler_init();

volatile TaskDescriptor* scheduler_get_running();

void scheduler_append(volatile TaskDescriptor *td);

int scheduler_empty();

volatile TaskDescriptor* scheduler_get();

void scheduler_set_running(volatile TaskDescriptor* td);

void scheduler_move2ready();

#endif // SCHEDULER_H_
