#ifndef RTX_H_
#define RTX_H_

#include <ts7200.h>

int handle_request_create(int priority, void(*code) () );
int handle_request_tid();
int handle_request_parent();
void handle_request_pass();
void handle_request_exit();

#endif
