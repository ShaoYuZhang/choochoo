#include "../keyboard_decoder_process/keyboard_decoder_process.h"
#include "../keyboard_decoder_process/registered_command.h"
#include "../message_passing/Mailbox.h"
#include "../shared/rtx.h"
#include "../shared/rtx_inc.h"
#include "../shared/dbug.h"
#include "ProcessABC.h"
#include "../crt_display/crt_display.h"

void user_process_a()
{
  { // Register the command
    Mail* envelop = (Mail*)request_memory_block();
    RegisteredCommand* rc = (RegisteredCommand*)(((char*)envelop)+64);
    RegisteredCommand_construct(rc, 'Z', PROCESS_A_PID, PROCESS_A_START_MSG_TYPE);
    envelop->m_messageType = COMMAND_REGISTRATION_MSG_TYPE;
    send_message(KEYBOARD_DECODER_PROCESS_PID, envelop);
  }

  while (1)
  {
    int sender = 0;
    Mail* msg = (Mail*)receive_message(&sender);
    if (msg->m_messageType == PROCESS_A_START_MSG_TYPE){
	
    	rtx_dbug_outs((CHAR *)"Process A start \r\n");

      release_memory_block(msg);
      break;
    }
    else{
      release_memory_block(msg);
    }
  }

  int num = 0;

  while (1)
  {
    rtx_dbug_outs((CHAR *) " Process A request memory \r\n");
	
    Mail* envelop = (Mail*) request_memory_block();
    envelop->m_messageType = COUNT_REPORT_MSG_TYPE;
    int * messageBody = (int*)(((char*)envelop)+64);
    (*messageBody) = num;

    send_message(PROCESS_B_PID, envelop);
    num++;
    release_processor();
  }
}

void user_process_b()
{
  while (1)
    {
      int sender = 0;
      void* msg = receive_message(&sender);
      send_message(PROCESS_C_PID, msg);
    }
}

void user_process_c()
{
  //construct a local message queue
  Mailbox local;
  Mailbox_construct(&local);
  
  
  rtx_dbug_outs((CHAR *)"process c init done");

  while (1)
  {
    // Try to dequeue from local
    UINT32 mail_senderId;
    Mail* msg = (Mail*)Mailbox_getMailOut(&local, &mail_senderId);

    // If empty, receive from process queue.
    if (msg == NULL){
	
		rtx_dbug_outs((CHAR *)"process c try to receive message");
  
      msg = (Mail*) receive_message((int*)&mail_senderId);

		rtx_dbug_outs((CHAR *)"process c not blocked");	  
	  
    }

    if (((Mail*)msg)->m_messageType == COUNT_REPORT_MSG_TYPE)
    {
      int* messageBody = (int*)(((char*)msg)+64);
      if ((*messageBody)%20 == 0)
      {
        msg->m_messageType = CRT_OUTPUT_MSG_TYPE;
        //send "Process C" to CRT Display using msg envelop msg
        char* process_c = (char*) messageBody;
        process_c[0] = 'P';
        process_c[1] = 'r';
        process_c[2] = 'o';
        process_c[3] = 'c';
        process_c[4] = 'e';
        process_c[5] = 's';
        process_c[6] = 's';
        process_c[7] = ' ';
        process_c[8] = 'C';
        process_c[9] = '\r';
		process_c[10] = '\n';
        process_c[11] = '\0'; // NULL terminated.
        send_message(CRT_DISPLAY_PID, msg);

        // hibernate for 10 sec
        Mail* delay = (Mail*)request_memory_block();
        delay->m_messageType = DELAY_SEND_MSG_TYPE;
        //request a delayed_send for 10 sec delay with msg_type=wakeup10 using q
        delayed_send(PROCESS_C_PID, delay, 10000);

        while(1)
        {
          msg = (Mail*)receive_message((int*)&mail_senderId);
          if (msg->m_messageType == DELAY_SEND_MSG_TYPE){
            break;
          }
          else{
            Mailbox_putMailIn(&local, msg);
          }
        }
      }
    }
    release_memory_block(msg);
    release_processor();
  }
}
