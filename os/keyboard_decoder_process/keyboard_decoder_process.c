#include "keyboard_decoder_process.h"
#include "registered_command.h"
#include "../memory_pool/MemoryPool.h"
#include "../message_passing/Mailbox.h"
#include "../shared/rtx.h"
#include "../crt_display/crt_display.h"


#define MAX_USER_INPUT_LENGTH 64
#define BACKSPACE 8

// Global variables for the keyboard decoder process
// Global to avoid passing variables.
RegisteredCommand* registered_commands_begin; // List of registered commands
CHAR   user_input [MAX_USER_INPUT_LENGTH];
UINT32 user_input_current_index;

// Prototypes
VOID register_command(RegisteredCommand* input);
VOID append_input(char* input);
UINT32 send_registered_command();

// ---------------------------------------------------------------------------
VOID keyboard_decoder_process()
{
  rtx_dbug_outs((CHAR *)"<=========Entering KCD");
  // Initializations
  int i;
  for (i = 0; i < MAX_USER_INPUT_LENGTH; i++)
  {
    user_input[i] = 0;
  }
  user_input_current_index = 0;
  registered_commands_begin = NULL;


  // Handling user input.
  while (TRUE)
  {
    int senderID;
    // Get a message can append it to the queue.
    char* msg = (char*)receive_message(&senderID);
    char* newChar = ((char*)msg)+64;
	
    Mail* mail = (Mail*)msg;
    if (mail->m_messageType == COMMAND_REGISTRATION_MSG_TYPE)
    {
	  rtx_dbug_outs("Registering command..\r\n");
      register_command((RegisteredCommand*)newChar);
	  
	  release_memory_block(msg); // Release memory used by sender.
    }
    else if (mail->m_messageType == INPUT_KEY_MSG_TYPE)
    {
      append_input(newChar);
	  
	  // NULL terminate the input for CRT
	  if(*newChar == '\r')
	  {
		newChar[1] = '\n';
		newChar[2] = '\0';
	  }
	  else{
		newChar[1] = '\0';
	  }
	  
	  mail->m_messageType = CRT_OUTPUT_MSG_TYPE;
      send_message(CRT_DISPLAY_PID, mail);
	  
      // Process input if we have a newline.
      if (*newChar == '\r'){
	    send_registered_command();
		user_input_current_index = 0;
      }
    }

    
  } // End infinite loop
}

// ---------------------------------------------------------------------------
VOID register_command(RegisteredCommand* input)
{
  RegisteredCommand* comm = (RegisteredCommand*)MemoryPool_alloc();
  // Copy input to comm
  RegisteredCommand_construct_copy(comm, input);

  // Append commands to front of list
  if (registered_commands_begin == NULL){
    registered_commands_begin = comm;
  }
  else {
    comm->m_next = registered_commands_begin;
    registered_commands_begin = comm;
  }
}

// ---------------------------------------------------------------------------
VOID append_input(char* input)
{
  // Process backspace
  if (*input == BACKSPACE)
  {
    if (user_input_current_index > 0){
      user_input_current_index -= 1;
    }
    return;
  }

  // Make sure we don't write beyond the buffer
  if (user_input_current_index == MAX_USER_INPUT_LENGTH){
    // Write to last location.. silently.
    user_input[MAX_USER_INPUT_LENGTH - 1] = *input;
  }
  else {
    user_input[user_input_current_index] = *input;
    user_input_current_index += 1;
  }
}

// ---------------------------------------------------------------------------
UINT32 send_registered_command()
{
  if (user_input[0] != '%'){
    return 1;
  }

  RegisteredCommand* rc = registered_commands_begin;

  // Loop through registered commands, send applicable messages.
  while (rc != NULL)
  {
    if (user_input[1] == rc->m_letter)
    {
      // Wall clock.
      Mail* mail = (Mail*)request_memory_block();
      mail->m_messageType = rc->m_messageType;
      char* msgContent = ((char*)mail)+64;
      rtx_dbug_outs("sending message to processor");
	  rtx_dbug_out_number(rc->m_senderId);
	  
      // Copy mail.
      int current;
      for (current = 0; current < MAX_USER_INPUT_LENGTH; current++)
      {
        msgContent[current] = user_input[current];
      }

      send_message(rc->m_senderId, (void*)mail);
    }

    rc = rc->m_next;
  }

  return 0;
}
// ---------------------------------------------------------------------------

