#ifndef IOSERVER_H_
#define IOSERVER_H_

#define IOSERVER_NAME "IO\0"

char GetcCOM2(const int tid);

void PutcCOM2(const int tid, const char c);

int startIoServerTask();

#endif // IOSERVER_H_
