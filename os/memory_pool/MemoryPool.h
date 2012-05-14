#ifndef MEMORY_POOL_h_
#define MEMORY_POOL_h_

#include "MemoryBlock.h"

typedef struct MemoryPool MemoryPool;
struct MemoryPool
{
  MemoryBlockList m_free_block_stack;
  MemoryBlockList m_used_block_stack;
};


VOID* MemoryPool_alloc();

VOID MemoryPool_free(VOID* block);

VOID MemoryPool_init();

#endif // MEMORY_POOL_h_
