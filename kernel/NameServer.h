#ifndef NAMESERVER_H_
#define NAMESERVER_H_

#include <syscall.h>

// Using magic numbers to avoid adding message type.
// Rely on null-termination for name.
#define REGISTER_AS 79
#define WHO_IS      81
#define NAMESERVER_TID 0

void startNameserver();

int whoIsNameserver();

int RegisterAs(char* name);

int WhoIs(char* name);

#endif // NAMESERVER_H_
