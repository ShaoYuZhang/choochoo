#include "wall_clock_process.h"
#include "../keyboard_decoder_process/keyboard_decoder_process.h"
#include "../keyboard_decoder_process/registered_command.h"
#include "../message_passing/Mailbox.h"
#include "../shared/rtx.h"
#include "../shared/rtx_inc.h"
#include "../crt_display/crt_display.h"

// Prototypes
UINT32 parse_to_sec(char* start);
void parse_to_display(int* seconds, char* display);

void wall_clock()
{

  { // Register the command.
    Mail* envelop = (Mail*)request_memory_block();
    RegisteredCommand* rc = (RegisteredCommand*)(((char*)envelop)+64);
    RegisteredCommand_construct(rc,'W', WALL_CLOCK_PID, WALL_CLOCK_CHANGE_MSG_TYPE);
    envelop->m_messageType = COMMAND_REGISTRATION_MSG_TYPE;

    send_message(KEYBOARD_DECODER_PROCESS_PID, envelop);
	//rtx_dbug_outs((CHAR *)"<=========register command complete");
  }

  Mail* secMail = (Mail*)request_memory_block();
  Mail* crtMessage = (Mail*)request_memory_block();
  secMail->m_messageType = WALL_CLOCK_NEXT_SECOND_MSG_TYPE;
  delayed_send(WALL_CLOCK_PID, secMail, 1000);
  //rtx_dbug_outs((CHAR *)"<=========delay send complete");

  // Start the wall clock.
  int currentTime = -1;
  while (TRUE)
  {
    int senderId;
	  
    secMail = (Mail*)receive_message(&senderId);
	
	    //rtx_dbug_outs((CHAR *)"received message! \r\n");
	
    char* msg = ((char*)secMail)+64;

    if (secMail->m_messageType == WALL_CLOCK_CHANGE_MSG_TYPE)
    {
	  // Parse the message.
      char type = msg[2];

      if (type == 'T')
      {
        currentTime = -1;
      }
      else if (type == 'S')
      {
        // Set wall clock
        // %WS HH:MM:SS
        currentTime = parse_to_sec(msg);
		
	    //rtx_dbug_outs((CHAR *)"clock set");
		//rtx_dbug_out_number(currentTime);
      }
	  
	  release_memory_block(secMail);
	  continue;
    }
    else if (secMail->m_messageType == WALL_CLOCK_NEXT_SECOND_MSG_TYPE)
    {
	  //rtx_dbug_outs((CHAR *)"clock add second\r\n");
      if (currentTime != -1){
        currentTime += 1;
		//rtx_dbug_outs((CHAR *)"clock add second\r\n");
		if (currentTime == 24*60*60){
		  currentTime = 0;
		}
      }
    }
	else {
		//rtx_dbug_outs((CHAR *)"BAD BAD\r\n");
	}

    char* data = ((char*)crtMessage)+64;
    if (currentTime != -1){
      parse_to_display(&currentTime, data);
      data[10] = '\0';
    }
    else {
	  data[0] = '\0';
    }
	delayed_send(WALL_CLOCK_PID, secMail, 1000);

    crtMessage->m_messageType = CRT_OUTPUT_MSG_TYPE;
    send_message(CRT_DISPLAY_PID, crtMessage);
	crtMessage = (Mail*)request_memory_block();
  }
}

// ------------------------------------------------------------------
UINT32 parse_to_sec(char* s)
{
  // s is in format %WS HH:MM:SS
  int hour = (s[4] - '0') * 10;
  hour += (s[5] - '0');

  int min = (s[7] - '0') *10;
  min += (s[8] - '0');

  int sec = (s[10] - '0') *10;
  sec += (s[11] - '0');

  if (hour < 24 && min < 60 && sec < 60 &&
      s[6] == ':' && s[9] == ':')
  {
    return hour*60*60 + min*60 + sec;
  }
  else {
    return -1;
  }
}

void fillChar ( char *start, int num) {
  // fill 2 digits of num into start
  int temp = num;
  int i;
  for (i = 1; i >= 0; i--) {
    start[i] = temp % 10 + '0';
    temp /= 10;
  }
  return;
}

void parse_to_display(int * seconds, char* display){
  int hr, min, sec;
  sec = (*seconds) % 60;
  min = (*seconds) / 60;
  hr = min / 60;
  min = min % 60;
  if (hr > 24) {
    hr = hr % 24;
    (*seconds) %= (60 * 60 * 24);
  }

  fillChar(display, hr);
  display[2] = ':';
  fillChar(display + 3, min);
  display[5] = ':';
  fillChar(display + 6, sec);
  display[8] = 0;

  return;
}

