#ifndef _DEFINE_TRAP_H_
#define _DEFINE_TRAP_H_

#define REQUEST_MEMORY_BLOCK_TRAP_CALL 2
#define RELEASE_MEMORY_BLOCK_TRAP_CALL 3
#define RELEASE_PROCESSOR_TRAP_CALL    4
#define SEND_MESSAGE_TRAP_CALL         5
#define RECEIVE_MESSAGE_TRAP_CALL      6
#define DELAY_SEND_TRAP_CALL           7
#define SET_PROCESS_PRIORITY_TRAP_CALL 8
#define GET_PROCESS_PRIORITY_TRAP_CALL 9

#define CALL_TRAP(trap_code, arg1, arg2, arg3, rtn) \
  UINT32* yyyyy = &rtn;           \
  /* Save registers on stack*/ \
  asm( "move.l %d0, -(%a7)" ); \
  asm( "move.l %d1, -(%a7)" ); \
  asm( "move.l %d2, -(%a7)" ); \
  asm( "move.l %d3, -(%a7)" ); \
  asm( "move.l %d4, -(%a7)" ); \
  /* Assign registers c_trap_handler will use */ \
  asm( "move.l %0, %%d1;"::"r"(arg1) );  \
  asm( "move.l %0, %%d2;"::"r"(arg2) );  \
  asm( "move.l %0, %%d3;"::"r"(arg3) );  \
  asm( "move.l %0, %%d4;"::"r"(yyyyy) ); \
  asm( "move.l #" str(trap_code) ", %d0" );      \
  asm( "TRAP #0" );                      \
  /* Restore registers from stack */     \
  asm( "move.l %d4, (%a7)+" ); \
  asm( "move.l %d3, (%a7)+" ); \
  asm( "move.l %d2, (%a7)+" ); \
  asm( "move.l %d1, (%a7)+" ); \
  asm( "move.l %d0, (%a7)+" );

// str(4) ==> "4"
#define str(s) #s

VOID c_trap_handler(SINT32 trapCode, UINT32 arg1, UINT32 arg2, UINT32 arg3, VOID* returnVal);


#endif // _DEFINE_TRAP_H_
