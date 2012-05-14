#include "../message_passing/Mailbox.h"
#include "../shared/rtx.h"
#include "../shared/dbug.h"
#include "../scheduler/scheduler.h"
#include "../shared/rtx_inc.h"
#include "../initialization/ProcessQueue.h"
#include "../keyboard_decoder_process/keyboard_decoder_process.h"
#include "../crt_display/crt_display.h"
#include "../atomic/atomic.h"

//#define _DEBUG_

VOID uart_i_process( VOID )
{
  //atomic_on();
  while (1) {
    BYTE temp;

    temp = SERIAL1_USR;    /* Ack the interrupt */

#ifdef _DEBUG_
    rtx_dbug_outs((CHAR *) "Enter: c_serial_handler ");
#endif /* _DEBUG_ */

    /* See if data is waiting.... */
    if( temp & 1 )
    {
#ifdef _DEBUG_
      rtx_dbug_outs((CHAR *) "reading data: ");
#endif /* _DEBUG_ */

      //Construct message and send KCD
      char CharIn = SERIAL1_RD;
      if (CharIn == '!') {
        //Output Hotkey
        dump_all();
      } else if (CharIn == '@') {
		// Our own debug message
		dump_queue();
	  }else {
        void* envelop = request_memory_block();
		if (envelop != NULL)
		{
			Mail* mail = (Mail*) envelop;
			char* message = (char*) envelop + 64;
	
			mail->m_messageType = INPUT_KEY_MSG_TYPE;
			*message = CharIn;
			send_message(KEYBOARD_DECODER_PROCESS_PID, envelop);
		}
      }
    }

    /* See if port is ready to accept data */
    else if ( temp & 4 )
    {
      int senderID;
      char* envelop = (char*)receive_message(&senderID);
      if (envelop != NULL) {
        char* message = envelop + 64;

        Mail* mail = (Mail*)envelop;
        if (senderID == CRT_DISPLAY_PID  && mail->m_messageType == UART_OUTPUT_CHAR_MSG_TYPE) {
          SERIAL1_WD = *message;   /* Write data to port */
		  #ifdef _DEBUG_
      rtx_dbug_outs((CHAR *) "writing data: ");
	  rtx_dbug_out_char(*message);
      rtx_dbug_outs((CHAR *) "\r\n");
#endif /* _DEBUG_*/
        }
		release_memory_block(envelop);
      }
	  else{
	    SERIAL1_IMR = 2;        /* Disable tx Interupt */
	  }
    }
    context_switch_back_from_i_process();
  }
}
