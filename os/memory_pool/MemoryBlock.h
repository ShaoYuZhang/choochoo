#ifndef OS_MemoryBlock_h_
#define OS_MemoryBlock_h_

#include "../shared/rtx_inc.h"

#define POOL_BLOCK_SIZE 32

typedef struct MemoryBlock {
  // Pointer to previous memory block.
  char m_data[POOL_BLOCK_SIZE];
  struct MemoryBlock* m_next;
  struct MemoryBlock* m_previous;
} MemoryBlock;

typedef struct MemoryBlockList {
  MemoryBlock* m_begin;
} MemoryBlockList;

// Prototypes

void MemoryBlockList_construct(MemoryBlockList* caller);

void MemoryBlockList_push_front(MemoryBlockList* caller, MemoryBlock* block);

void MemoryBlockList_push_back(MemoryBlockList* caller, MemoryBlock* block);

MemoryBlock* MemoryBlockList_pop_front(MemoryBlockList* caller);

void MemoryBlockList_erase(MemoryBlockList* caller, MemoryBlock* block);

BOOLEAN MemoryBlockList_empty(MemoryBlockList* caller);

SIZE_T MemoryBlockList_size(MemoryBlockList* caller);

#endif // OS_MemoryBlock_h_
