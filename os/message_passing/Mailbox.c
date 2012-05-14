#include "Mailbox.h"
#include "../memory_pool/MemoryPool.h"
#include "../shared/rtx.h"
#include "../scheduler/scheduler.h"

// ---------------------------------------------------------------------------
VOID Mailbox_construct(Mailbox* caller)
{
  caller->m_mail = NULL;
}

// ---------------------------------------------------------------------------
VOID Mailbox_putMailIn(Mailbox* caller, VOID* message)
{
  // Package memory for message.

  Mail* mail = (Mail*)message;
  //  _______________________________________________________________
  // | message_ptr | process_id_of_caller  | ... | 64  byte user msg |
  // |_____________|_______________________|_____|___________________|
  //
  mail->m_senderID = get_running_process()->pid;
  mail->m_next_mail = NULL;

  // put mail in last location
  Mail* current = caller->m_mail;
  Mail* previous = NULL;

  while (current != NULL){
    previous = current;
    current = current->m_next_mail;
  }

  // Current last node's next is now next;
  if (previous == NULL){
    // empty
    caller->m_mail = mail;
  }
  else {
    previous->m_next_mail = mail;
  }
}

// ---------------------------------------------------------------------------
VOID* Mailbox_getMailOut(Mailbox* caller, UINT32* sender_id)
{
  // While no mail
  if (caller->m_mail == NULL)
  {
    return NULL;
  }

  // Mailbox will have at least one message now.
  Mail* mail = (Mail*)caller->m_mail;
  if (mail->m_delayed){
    *sender_id = mail->m_delaySenderID;
  }
  else {
    *sender_id = mail->m_senderID;
  }
  caller->m_mail = caller->m_mail->m_next_mail;

  return (VOID*)mail;
}

// ---------------------------------------------------------------------------
// Peek at the first element: used by timer i-process
Mail * Mailbox_peek(Mailbox* caller){
  return caller->m_mail;
}

// ---------------------------------------------------------------------------
// Insert Mail into Mailbox by firing Time: used by timer i-process
VOID Mailbox_insertMailByTime(Mailbox* caller, VOID* message){
  Mail* cur = Mailbox_peek(caller);
  UINT32 message_time = ((Mail*)message)->m_firingTime;
  if(cur == NULL){
    Mailbox_putMailIn(caller, message);
    return;
  }else{
    if (message_time >= cur->m_firingTime && cur->m_next_mail == NULL){
      Mailbox_putMailIn(caller, message);
      return;
    }else if(message_time < cur->m_firingTime){
      caller->m_mail = (Mail*) message;
      ((Mail*)message)->m_next_mail = cur;
      return;
    }else{
      while(cur!=NULL && cur->m_next_mail != NULL){
        if(message_time < cur->m_next_mail->m_firingTime){
          Mail* temp = cur->m_next_mail;
          cur->m_next_mail = (Mail*)message;
          ((Mail*)message)->m_next_mail = temp;
          return;
        }
        cur = cur->m_next_mail;
      }
      Mailbox_putMailIn(caller, message);
      return;
    }
  }
}

