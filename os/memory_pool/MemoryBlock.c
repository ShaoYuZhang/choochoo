#include "MemoryBlock.h"

// --------------------------------------------------------------------------
void MemoryBlockList_construct(MemoryBlockList* caller){
  caller->m_begin = NULL;
}

// --------------------------------------------------------------------------
void MemoryBlockList_push_front(MemoryBlockList* caller, MemoryBlock* block)
{
  MemoryBlock* tmp = caller->m_begin;
  if (tmp != NULL){
    tmp->m_previous = block;
  }

  caller->m_begin = block;
  caller->m_begin->m_next = tmp;
  caller->m_begin->m_previous = NULL;
}

// --------------------------------------------------------------------------
void MemoryBlockList_push_back(MemoryBlockList* caller, MemoryBlock* block)
{
  // TODO TEST
  MemoryBlock* current = caller->m_begin;
  MemoryBlock* second = NULL;

  while (current != NULL){
    second = current;
    current = current->m_next;
  }

  // Current last node's next is now next;
  if (second != NULL){
    second->m_next = block;
  }
  else {
    // empty
    caller->m_begin = block;
  }
  block->m_previous = second;
  block->m_next     = NULL;
}


// --------------------------------------------------------------------------
MemoryBlock* MemoryBlockList_pop_front(MemoryBlockList* caller)
{
  MemoryBlock* tmp = caller->m_begin;
  caller->m_begin = caller->m_begin->m_next;
  if (caller->m_begin != NULL){
    caller->m_begin->m_previous = NULL;
  }
  return tmp;
}

// --------------------------------------------------------------------------
void MemoryBlockList_erase(MemoryBlockList* caller, MemoryBlock* block)
{
  if (block->m_previous == NULL)
  {
    caller->m_begin = block->m_next;
    if (block->m_next != NULL)
    {
      caller->m_begin->m_previous = NULL;
    }
  }
  else {
    block->m_previous->m_next = block->m_next;
    if (block->m_next != NULL){
      block->m_next->m_previous = block->m_previous;
    }
  }
};

// --------------------------------------------------------------------------
BOOLEAN MemoryBlockList_empty(MemoryBlockList* caller)
{
  return (caller->m_begin == NULL);
}

// --------------------------------------------------------------------------
SIZE_T MemoryBlockList_size(MemoryBlockList* caller)
{
  SIZE_T count = 0;
  MemoryBlock* current = caller->m_begin;
  while (current != NULL)
  {
    count+=1;
    current = current->m_next;
  }
  return count;
}
// --------------------------------------------------------------------------
