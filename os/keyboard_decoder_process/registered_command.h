#ifndef registered_command_h_
#define registered_command_h_

#include "../shared/rtx_inc.h"

typedef struct RegisteredCommand
{
  char m_letter;
  UINT32 m_senderId;
  UINT32 m_messageType;

  struct RegisteredCommand* m_next;
} RegisteredCommand;

VOID RegisteredCommand_construct(RegisteredCommand* caller,
                                 char letter,
                                 UINT32 senderID,
                                 UINT32 messageType);

// Copy constructor
VOID RegisteredCommand_construct_copy(RegisteredCommand* caller, RegisteredCommand* other);

#endif // registered_command_h_

