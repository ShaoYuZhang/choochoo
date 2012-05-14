#ifndef Mailbox_h_
#define Mailbox_h_

#include "../shared/rtx_inc.h"
#include "../memory_pool/MemoryBlock.h"

typedef struct Mail
{
  UINT32 m_senderID;
  UINT32 m_destinationID;
  UINT32 m_messageType;
  UINT32 m_delayTime;

  //used by timer i-process
  UINT32 m_firingTime;
  BOOLEAN m_delayed;
  UINT32 m_delaySenderID;

  struct Mail*  m_next_mail;
} Mail;

// Mailbox is a struct held by each process. (It stores messages sent to the process)
typedef struct Mailbox
{
  Mail* m_mail; // List of user_heap envelops.
} Mailbox;

// Initial mailbox.
VOID Mailbox_construct(Mailbox* caller);

// Send a mail packaged by a process, message point to one of the 128byte block.
// Non-blocking.
VOID Mailbox_putMailIn(Mailbox* caller, VOID* message);

// Get a mail from this caller process's mailbox.  Blocking
// RETURN: pointer to one of the 128byte block.
VOID* Mailbox_getMailOut(Mailbox* caller, UINT32* sender_id);

// Peek at the first element: used by timer i-process
Mail * Mailbox_peek(Mailbox* caller);

// Insert Mail into Mailbox by firing Time: used by timer i-process
VOID Mailbox_insertMailByTime(Mailbox* caller, VOID* message);


#endif // Mailbox_h_
