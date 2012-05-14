#include "crt_display.h"
#include "../shared/rtx.h"
#include "../shared/rtx_inc.h"
#include "../shared/dbug.h"
#include "../message_passing/Mailbox.h"
#include "../wall_clock/wall_clock_process.h"
#include "../initialization/InitTable.h"

#define MAX_USER_INPUT_LENGTH 84 //64 + magic number

void crt_display_process()
{
  CHAR buffer[MAX_USER_INPUT_LENGTH];
  int bufferIter = 0;

  //initially, move cursor down one line to save space for clock
  buffer[bufferIter++] = ESC;
  buffer[bufferIter++] = '[';
  buffer[bufferIter++] = '1';
  buffer[bufferIter++] = 'B';

  buffer[bufferIter++] = ESC;
  buffer[bufferIter++] = '[';
  buffer[bufferIter++] = 's';

  Mail* mailToSend = (Mail*)request_memory_block();
  while(TRUE)
  {
    int sender = 0;
    Mail *mail = (Mail *)receive_message(&sender);
	//rtx_dbug_outs("CRT enterrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrred");
    int messageType = mail-> m_messageType;
    if (messageType == CRT_OUTPUT_MSG_TYPE)
    {
      //move cursor to top
      if (sender == WALL_CLOCK_PID)
      {
        buffer[bufferIter++] = ESC;
        buffer[bufferIter++] = '[';
        buffer[bufferIter++] = '0';
        buffer[bufferIter++] = ';';
        buffer[bufferIter++] = '0';
        buffer[bufferIter++] = 'H';
		buffer[bufferIter++] = ESC;
        buffer[bufferIter++] = '[';
        buffer[bufferIter++] = 'K';
      }
      //restore cursor position
      else
      {
        buffer[bufferIter++] = ESC;
        buffer[bufferIter++] = '[';
        buffer[bufferIter++] = 'u';
      }
      char *startOfString = ((char*)mail) +64;

      char *iterator = startOfString;
      while (*iterator != 0)
      {
        buffer[bufferIter++] = *iterator;
        iterator++;
      }
      //save cursor position
      if (sender != WALL_CLOCK_PID)
      {
        buffer[bufferIter++] = ESC;
        buffer[bufferIter++] = '[';
        buffer[bufferIter++] = 's';
      }
    }
#ifdef _DEBUG
    int returnCode =   
#endif
    release_memory_block(mail);


#ifdef _DEBUG
    if (returnCode != 0)
    {
      //rtx_dbug_outs((CHAR *)"release memory failed\r\n");
    }
#endif

    int i;
    for (i = 0; i < bufferIter; i++)
    {
      

      mailToSend->m_messageType = UART_OUTPUT_CHAR_MSG_TYPE;
      char *messageInMail = ((char *)mailToSend + 64);
      *messageInMail = buffer[i]; 
      send_message(UART_I_PROCESS_PID, mailToSend); 
      SERIAL1_IMR = 3;
	  mailToSend = (Mail*)request_memory_block();
    }

    bufferIter = 0;
  }
}
