/**
 * @file: timer.c
 * @brief: timer 0 smaple code
 * @author: ECE354 Lab Instructors and TAs
 * @author: Irene Huang
 * @date: 2011/01/04
 */

#include "../shared/rtx_inc.h"
#include "../dbug/dbug.h"

/*
 * Global Variables
 */
volatile SINT32 Counter = 0;
volatile BYTE CharOut = '\0';
volatile BOOLEAN Caught = FALSE;
volatile BOOLEAN clockCounterChanged = TRUE;

/*
 * gcc expects this function to exist
 */
int __main( void )
{
    return 0;
}

VOID c_serial_handler( VOID )
{
    BYTE temp;

    temp = SERIAL1_USR;    /* Ack the interrupt */

#ifdef _DEBUG_
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *) "Enter: c_serial_handler ");
#endif

#endif /* _DEBUG_ */

  if ( temp & 4 )
  {
	Caught = FALSE;
#ifdef _DEBUG_
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *) "writing data: ");
#endif

 #ifdef _DEBUG
    rtx_dbug_out_char(CharOut);
#endif

    tx_dbug_outs((CHAR *) "\r\n");
#endif /* _DEBUG_*/
    SERIAL1_WD = CharOut;   /* Write data to port */
    SERIAL1_IMR = 2;        /* Disable tx Interupt */
  }
    return;
}

void fillChar ( char *start, int num) {
  // fill 2 digits of num into start
  int temp = num;
  int i;
  for (i = 1; i >= 0; i--) {
    start[i] = temp % 10 + '0';
    temp /= 10;
  }
}

void outString ( char * str ) {
  CharOut = '\r';
  SERIAL1_IMR = 3;
  
  while (*str != 0) {
     if( !Caught )
     {
        Caught = TRUE;
        CharOut = *str;
        SERIAL1_IMR = 3;

        str ++;
     }
  }
}

/*
 * This function is called by the assembly STUB function
 */
VOID c_timer_handler( VOID )
{
  Counter++;
	clockCounterChanged = TRUE;

	//Ack interrupt
	TIMER0_TER = 2;
}

VOID print_clock(int currentClockCount)
{
    int hr, min, sec;
    sec = Counter % 60;
    min = Counter / 60;
    hr = min / 60;
    min = min % 60;
    if (hr > 24) {
      hr = hr % 24;
      Counter %= (60 * 60 * 24);
    }

    char tStr[9];
    fillChar(tStr, hr);
    tStr[2] = ':';
    fillChar(tStr + 3, min);
    tStr[5] = ':';
    fillChar(tStr + 6, sec);
    tStr[8] = 0;

    outString(tStr);
}




SINT32 coldfire_vbr_init( VOID )
{
    /*
     * Move the VBR into real memory
     */
    asm( "move.l %a0, -(%a7)" );
    asm( "move.l #0x10000000, %a0 " );
    asm( "movec.l %a0, %vbr" );
    asm( "move.l (%a7)+, %a0" );
    
    return RTX_SUCCESS;
}



/*
 * Entry point, check with m68k-coff-nm
 */
int main( void )
{
    UINT32 mask;

    /* Disable all interupts */
    asm( "move.w #0x2700,%sr" );

    coldfire_vbr_init();

    /*
     * Store the serial ISR at user vector #64
     */
    asm( "move.l #asm_serial_entry,%d0" );
    asm( "move.l %d0,0x10000100" );

    /* Reset the entire UART */
    SERIAL1_UCR = 0x10;

    /* Reset the receiver */
    SERIAL1_UCR = 0x20;
    
    /* Reset the transmitter */
    SERIAL1_UCR = 0x30;

    /* Reset the error condition */
    SERIAL1_UCR = 0x40;

    /* Install the interupt */
    SERIAL1_ICR = 0x17;
    SERIAL1_IVR = 64;

    /* enable interrupts on rx only */
    SERIAL1_IMR = 0x02;

    /* Set the baud rate */
    SERIAL1_UBG1 = 0x00;
#ifdef _CFSERVER_           /* add -D_CFSERVER_ for cf-server build */
    SERIAL1_UBG2 = 0x49;    /* cf-server baud rate 19200 */ 
#else
    SERIAL1_UBG2 = 0x92;    /* lab board baud rate 9600 */
#endif /* _CFSERVER_ */

    /* Set clock mode */
    SERIAL1_UCSR = 0xDD;

    /* Setup the UART (no parity, 8 bits ) */
    SERIAL1_UMR = 0x13;
    
    /* Setup the rest of the UART (noecho, 1 stop bit ) */
    SERIAL1_UMR = 0x07;

    /* Setup for transmit and receive */
    SERIAL1_UCR = 0x05;

    /*
     * Store the timer ISR at auto-vector #6
     */
    asm( "move.l #asm_timer_entry,%d0" );
    asm( "move.l %d0,0x10000078" );

    /*
     * Setup to use auto-vectored interupt level 6, priority 3
     */
    TIMER0_ICR = 0x9B;

    /*
     * Set the reference counts, ~10ms
     */
    TIMER0_TRR = 176;

    /*
     * Setup the timer prescaler and stuff
     */
    TIMER0_TMR = 0xFF1B;

    /*
     * Set the interupt mask
     */
    mask = SIM_IMR;
    mask &= 0x0003ddff;
    SIM_IMR = mask;    

    Counter=0;

    /* Let the timer interrupt fire, lower running priority */
    asm( "move.w #0x2000,%sr" );

    while (1)
	{
		if (clockCounterChanged)
		{
			print_clock(Counter);
			clockCounterChanged = FALSE;
		}
	}

    return 0;
}
