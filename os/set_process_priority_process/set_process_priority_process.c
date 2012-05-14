#include "set_process_priority_process.h"

#include "../keyboard_decoder_process/keyboard_decoder_process.h"
#include "../keyboard_decoder_process/registered_command.h"
#include "../message_passing/Mailbox.h"
#include "../shared/rtx.h"
#include "../crt_display/crt_display.h"

char SPPP_ERROR_MSG[] = "Change priority failed! Check param processId and priority.\0";

// ------------------------------------------------------------------
void set_process_priority_process()
{
  { // Register the command with keyboard decoder
    Mail* envelop = (Mail*)request_memory_block();
    RegisteredCommand* rc = (RegisteredCommand*)(((char*)envelop)+64);
    RegisteredCommand_construct(rc,'C',
        SET_PROCESS_PRIORITY_PROCESS_PID,
        SET_PROCESS_PRIORITY_PROCESS_MSG_TYPE);

    envelop->m_messageType = COMMAND_REGISTRATION_MSG_TYPE;

    send_message(KEYBOARD_DECODER_PROCESS_PID, envelop);
  }



  while (TRUE)
  {
    int senderId;
    Mail* envelop = (Mail*) receive_message(&senderId);
    char* msg = ((char*)envelop)+64;

	// msg of the form %C processId newPriority.
    // Skip whitespace.
    int current = 2;
    while (msg[current] == ' '){
      current += 1;
    }

    // Getting PID
    int pid = 0;
    while (msg[current] != ' ')
    {
	  pid *= 10;
      pid += (msg[current] - '0');
      current += 1;
    }

    // Skip whitespace.
    while (msg[current] == ' '){
      current += 1;
    }
	
    // Getting new priority
    int new_priority = 0;
    while(msg[current] != ' ' && msg[current] != '\r')
    {
      new_priority *= 10;
      new_priority += (msg[current] - '0');
      current += 1;
    }
	#if DEBUG_
    rtx_dbug_outs((CHAR*)"Setting priority\r\n");
	rtx_dbug_out_number(new_priority);
	rtx_dbug_out_number(pid);
	#endif 

    // Set the priority.
    if (set_process_priority(pid, new_priority) != 0)
    {
      // Failed
      envelop->m_messageType = CRT_OUTPUT_MSG_TYPE;
      char* data = ((char*)envelop)+64;
      int i;
      for(i = 0; i < 60; i++)
      {
        data[i] = SPPP_ERROR_MSG[i];
      }
      send_message(CRT_DISPLAY_PID, envelop);
    }
    else {
      release_memory_block(envelop);
    }
  }
}

