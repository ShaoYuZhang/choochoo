#ifndef IOSERVER_H_
#define IOSERVER_H_

#define IOSERVERCOM1_NAME "IOCOM1\0"
#define IOSERVERCOM2_NAME "IOCOM2\0"

typedef struct IOMessage {
  char type;
  char data;
} IOMessage;

typedef struct IOMessageStr {
  char type;
  char data[32];
} IOMessageStr;

char Getc(const int tid);

void Putc(const int tid, const char c);

void Putstr(const int tid, char* str, int len);

void startIoServerTask();

#endif // IOSERVER_H_
