#ifndef IOBUFFER_H_
#define IOBUFFER_H_

#define IO_BUFFER_SIZE 1024

typedef struct IOBuffer{
  char data[IO_BUFFER_SIZE];
  unsigned int bufferHead;
  unsigned int bufferTail;
} IOBuffer;

unsigned int increment_offset( int curr_offset );
void add_to_buffer( IOBuffer *buffer, char c );
int buffer_empty( IOBuffer *buffer );
char remove_from_buffer( IOBuffer *buffer );

#endif // IOBUFFER_H_
