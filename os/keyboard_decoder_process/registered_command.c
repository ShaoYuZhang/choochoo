#include "registered_command.h"

// construct
VOID RegisteredCommand_construct(RegisteredCommand* caller,
                                 char letter,
                                 UINT32 senderId,
                                 UINT32 messageType)
{
  caller->m_letter = letter;
  caller->m_senderId = senderId;
  caller->m_messageType = messageType;
  caller->m_next = NULL;
}

// copy construct
VOID RegisteredCommand_construct_copy(RegisteredCommand* caller, RegisteredCommand* other)
{
  caller->m_letter = other->m_letter;
  caller->m_senderId = other->m_senderId;
  caller->m_messageType = other->m_messageType;
  caller->m_next = NULL;
}
