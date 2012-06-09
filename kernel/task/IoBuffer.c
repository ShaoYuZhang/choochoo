#include <IoBuffer.h>

unsigned int increment_offset( int curr_offset ) {
    curr_offset++;
    if (curr_offset == IO_BUFFER_SIZE) {
        curr_offset = 0;
    }
    return curr_offset;
}

void add_to_buffer( IOBuffer *buffer, char c) {
  unsigned int temp_new_head = increment_offset(buffer->bufferHead);
  if (temp_new_head != buffer->bufferTail) {
    buffer->data[buffer->bufferHead] = c;
    buffer->bufferHead = temp_new_head;
  }
}

int buffer_empty( IOBuffer *buffer ) {
  return buffer->bufferHead == buffer->bufferTail;
}

char remove_from_buffer( IOBuffer *buffer) {
  if (buffer->bufferHead != buffer->bufferTail) {
    char c = buffer->data[buffer->bufferTail];
    buffer->bufferTail = increment_offset(buffer->bufferTail);
    return c;
  } else {
    return 255;
  }
}
