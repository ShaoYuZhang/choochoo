#ifndef KERNEL_PRIMITIVE_H
#define KERNEL_PRIMITIVE_H

int create (int priority, void (*code)());
int myTid ();
int myParentTid();
void pass();
void exit();

#endif
