#ifndef NAMESERVER_H_
#define NAMESERVER_H_

#include <syscall.h>

#define REGISTER_AS  79
#define WHO_IS       81
#define INSPECT      91
#define NAMESERVER_TID 2

void startNameServerTask();

int RegisterAs(char* name);

int WhoIs(char* name);

void PrintAll();

#endif // NAMESERVER_H_
