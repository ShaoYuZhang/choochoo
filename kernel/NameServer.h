#ifndef NAMESERVER_H_
#define NAMESERVER_H_

#include <syscall.h>

#define REGISTER_AS  79
#define WHO_IS       81
#define NAMESERVER_TID 0

void startNameserver();

int RegisterAs(char* name);

int WhoIs(char* name);

#endif // NAMESERVER_H_
