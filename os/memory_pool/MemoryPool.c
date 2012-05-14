#ifndef OS_MemoryPool_h_
#define OS_MemoryPool_h_

#include "MemoryPool.h"
#include "../shared/rtx.h"
#define POOL_SIZE 8192

// Global variable.
MemoryPool* g_pool;

VOID MemoryPool_construct(CHAR* pool, SIZE_T pool_size);

VOID MemoryPool_init()
{
  CHAR* pool = request_free_memory(POOL_SIZE);

  MemoryPool_construct(pool, POOL_SIZE);
}

// ---------------------------------------------------------------------------
VOID MemoryPool_construct(CHAR* pool, SIZE_T pool_size)
{
  g_pool = (MemoryPool*) pool;

  MemoryBlockList_construct(&(g_pool->m_free_block_stack));
  MemoryBlockList_construct(&(g_pool->m_used_block_stack));

  // Initialize block of memory specified by pool ptr and size of pool
  CHAR* start = pool + sizeof(MemoryPool);
  CHAR* end = pool + pool_size;

  while (start < end)
  {
    MemoryBlockList_push_front(&(g_pool->m_free_block_stack), (MemoryBlock*)start);
    start += sizeof(MemoryBlock);
  }
}

// ---------------------------------------------------------------------------
SIZE_T MemoryPool_num_free_blocks_(MemoryPool* caller)
{ return MemoryBlockList_size(&(caller->m_free_block_stack)); }
SIZE_T MemoryPool_num_free_blocks(){
  return MemoryPool_num_free_blocks_(g_pool);
}

// ---------------------------------------------------------------------------
SIZE_T MemoryPool_num_used_blocks_(MemoryPool* caller)
{ return MemoryBlockList_size(&(caller->m_used_block_stack)); }
SIZE_T MemoryPool_num_used_blocks()
{ return MemoryPool_num_used_blocks_(g_pool);}

// ---------------------------------------------------------------------------
// alloc, only give you block of one size.
void* MemoryPool_alloc_(MemoryPool* caller)
{
  if (MemoryBlockList_empty(&(caller->m_free_block_stack))){
    return NULL;
  }
  else
  {
    MemoryBlock* tmp = MemoryBlockList_pop_front(&(caller->m_free_block_stack));

    MemoryBlockList_push_front(&(caller->m_used_block_stack),
        (MemoryBlock*)tmp);
    return tmp;
  }
}
void* MemoryPool_alloc(){
  return MemoryPool_alloc_(g_pool);
}

// ---------------------------------------------------------------------------
// Warning no error checking is done. Better be sure what you're doing.
void MemoryPool_free_(MemoryPool*caller,void* block)
{
  MemoryBlockList_erase(&(caller->m_used_block_stack),
      (MemoryBlock*)block);
  MemoryBlockList_push_front(&(caller->m_free_block_stack),
      (MemoryBlock*)block);
}
void MemoryPool_free(void* block)
{
  return MemoryPool_free_(g_pool, block);
}

#endif // OS_MemoryPool_h_
