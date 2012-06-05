#ifndef IOSERVER_H_
#define IOSERVER_H_

#define IOSERVER_NAME "IO\0"

char Getc(const int channel);

void Putc(const int channel, const char c);

int startIoServerTask();

#endif // IOSERVER_H_
