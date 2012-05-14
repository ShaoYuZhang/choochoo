/**
 * @author:
 * @brief: ECE 354 S10 RTX Project P1-(c)
 * @date: 2011/01/07
 */

#include "../shared/rtx_inc.h"
#include "../shared/rtx.h"
#include "memory.h"
#include "../memory_pool/MemoryBlock.h"

#define BLOCK_SIZE 128
#define NUM_BLOCKS 32

// pointer to begin of 32 blocks of 128 bytes.
char* heap;// = NULL;
SINT32 free_memory;// = 0;


/*
 * @brief memory management initialization routine
 */
SINT32 getMask(int num)
{
  return 0x1 << num;
}

void init_user_heap()
{
  heap = request_free_memory(BLOCK_SIZE * NUM_BLOCKS);

  // All blocks are available
  free_memory = 0xFFFFFFFF;
}

/**
 * @brief request a free memory block of size at least 128B
 * @return starting address of the memory block
 *         and NULL on failure
 */

void* s_request_memory_block()
{
  int i;

  for (i = 0; i < NUM_BLOCKS; i++)
  {
    if ((free_memory & getMask(i)) != 0x00000000)
    {
      free_memory ^= getMask(i);// Set the i-th bit to 0
      return (void*)(heap + BLOCK_SIZE * i);
    }
  }
  return (void*)NULL; //nothing available
}

/**
 * @param: address of a memory block
 * @return: 0 on success, non-zero otherwise
 */
int s_release_memory_block( void* addr)
{
  int i = ((char*)addr - heap) / BLOCK_SIZE;
  int r = ((char*)addr - heap) % BLOCK_SIZE;

  if (i >= 0 && i < NUM_BLOCKS && r == 0)
  {
    free_memory |= getMask(i); // Set the i-th bit to 0
    return 0;
  }

  // Cannot free something that is not on the heap.
  return -1;
}
