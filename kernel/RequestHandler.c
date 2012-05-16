#include <TaskDescriptor.h>
#include <TaskQueue.h>
#include <Scheduler.h>

#include <RequestHandler.h>

int handle_request_create(int priority, void(*code) () ) {
    TaskDescriptor* td = get_running_task();
    //TODO ????????

    return 0;
}

int handle_request_tid() {
    TaskDescriptor* td = get_running_task();
    return td->tid;
}

int handle_request_parent() {
    TaskDescriptor* td = get_running_task();

    //TODO ????????

    return 0;
}

void handle_request_pass() {
    TaskDescriptor* td = get_running_task();
    td->state = READY;
    append_task(td);
}

void handle_request_exit() {
    TaskDescriptor* td = get_running_task();
    td->state = ZOMBIE;

    //TODO anything else??
}
