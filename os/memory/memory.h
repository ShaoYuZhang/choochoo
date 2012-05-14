#ifndef MEMORY_H_
#define MEMORY_H_

/**
 * @author:
 * @brief: ECE 354 S10 RTX Project P1-(c)
 * @date: 2011/01/07
 */

// Prototypes
void  init_user_heap();
void* s_request_memory_block();
int   s_release_memory_block( void* memory_block );

#endif // MEMORY_H_
