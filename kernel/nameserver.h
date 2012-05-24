#ifndef NAMESERVER_H_
#define NAMESERVER_H_

// Using magic numbers to avoid adding message type.
// Rely on null-termination for name.
#define REGISTER_AS 79
#define WHO_IS      81

void startNameserver();

int whoIsNameserver();

inline int RegisterAs(char* name) {
  char* reply[64];
  int r = Send(whoIsNameserver(), name, REGISTER_AS, reply, 64);
  return *(reply+4);
}

inline int WhoIs(char* name) {
  char* reply[64];
  int r = Send(whoIsNameserver(), name, WHO_IS, reply, 64);
  return *(reply+4);
}

#endif // NAMESERVER_H_
