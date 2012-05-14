#include "MemoryBlock.c"
#include "MemoryPool.c"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

char* freeMemoryBegin;

int main()
{
  size_t num_block = 10;
  char* pool = malloc(sizeof(MemoryPool) + num_block * sizeof(MemoryBlock));

  MemoryPool_construct(pool, num_block * sizeof(MemoryBlock));

  printf("%d\n", (int)MemoryPool_num_free_blocks());
  assert(MemoryPool_num_free_blocks() == num_block);
  assert(MemoryPool_num_used_blocks() == 0);

  int i;
  void* ptrs[num_block];

  for(i = 0; i < num_block; i++)
  {
    ptrs[i] = MemoryPool_alloc();
    assert(MemoryPool_num_free_blocks() == (num_block-i-1));
    assert(MemoryPool_num_used_blocks() == (i+1));
  }

  void* null_test = MemoryPool_alloc();
  assert(MemoryPool_num_free_blocks() == 0);
  assert(MemoryPool_num_used_blocks() == num_block);

  for(i = 0; i < num_block; i++)
  {
    MemoryPool_free(ptrs[i]);
    assert(MemoryPool_num_free_blocks() == (i+1));
    assert(MemoryPool_num_used_blocks() == (num_block-i-1));
  }

}

