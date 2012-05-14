#include <memory.h>

volatile char* freeMemoryBegin;

// Return a pointer to start of block of num_bytes size.
void* request_free_memory(long int num_bytes)
{
  if(num_bytes <= 0){
    return NULL;
  }

  // Round number of bytes to the smallest factor of 4.
  int diff = num_bytes%4;
  if (diff != 0) num_bytes += (4 - diff);

  void* tmp = (void*)freeMemoryBegin;
  freeMemoryBegin += num_bytes;

  return tmp;
}
