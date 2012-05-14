#include "../message_passing/Mailbox.h"
#include "../shared/rtx_inc.h"
#include "../shared/rtx.h"
#include "../shared/dbug.h"
#include "../wall_clock/wall_clock_process.h"
#include "../scheduler/scheduler.h"
#include "../atomic/atomic.h"

/*
 * Global Variables
 */


/*
 * This function is called by the assembly STUB function
 */
VOID timer_i_process( VOID )
{
  //atomic_on();
  //initialize a local mailbox
  //rtxdbug_outs("entering timer aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  Mailbox delay_send_queue;
  Mailbox_construct(&delay_send_queue);
  int sender;
  Mail* msg;
  void * envelop;
  int currentTime = 0;
  //rtx_dbug_outs("entering timer");
  while (1){
	//Ack interrupt
    TIMER0_TER = 2;
     //increment current time
    currentTime++;
	//rtx_dbug_outs("entering timer P2");
    //receive messages and manage delay send messages
	//rtx_dbug_outs("entering timer aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    msg = (Mail *)receive_message( &sender );
    while ( msg != NULL ) {
      ((Mail*)msg)->m_firingTime = currentTime + ((Mail*)msg)->m_delayTime;
	  //rtx_dbug_outs("entering timer bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
	  //rtx_dbug_out_number(((Mail*)msg)->m_firingTime );
	  
      if ( ((Mail*)msg)->m_delayed == TRUE) {
        Mailbox_insertMailByTime(&delay_send_queue, (VOID *)msg );
      }
      msg = (Mail *)receive_message( &sender );
    }

	//rtx_dbug_outs("entering timer P3");
    //send the messages that reached firingTime
    msg = Mailbox_peek(&delay_send_queue);
    while ( msg != NULL && ((Mail*)msg)->m_firingTime <= currentTime ) {
	  //rtx_dbug_outs("fire !!");
	  //rtx_dbug_out_number(((Mail*)msg)->m_firingTime );
      msg = ((Mail*)msg)->m_next_mail;
	  UINT32 sender_ID;
      envelop = Mailbox_getMailOut( &delay_send_queue, &sender_ID );
	  UINT32 destination = ((Mail*)envelop)->m_destinationID;
	  
	  //rtx_dbug_outs("send delayed mail to: pid ");
	  //rtx_dbug_out_number((int*)destination);
      send_message( destination, envelop );
    }

	//rtx_dbug_outs("exiting");
    context_switch_back_from_i_process();
  }

}



