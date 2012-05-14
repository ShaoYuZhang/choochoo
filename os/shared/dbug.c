/**
 * @file: dbug.c
 * @brief: output to janusROM terminal by using trap #15
 * @author: ECE354 Lab Instructors and TAs
 * @author: Irene Huang
 * @date: 2010/05/03
 */

#include "dbug.h"

/**
 * @brief: C Function wrapper for TRAP #15 function to output a character
 * @param: c the charcter to output to janusROM  
 */

VOID rtx_dbug_out_char( CHAR c )
{
	
    /* Store registers */
    asm( "move.l %d0, -(%a7)" );
    asm( "move.l %d1, -(%a7)" );

    /* Load CHAR c into d1 */
    asm( "move.l 8(%a6), %d1" );  /* Standard Motorola syntax */ 
    //asm( "move.l (8, %a6), %d1" );/* Standard Motorola syntax */
    //asm( "move.l %a6@(8), %d1" ); /* Motorola 680x0 syntax developed by MIT */

    /*
    asm("move.l %0, %%d1"
       : // no output  
       :"g"(c)
       :"d1" 
       );
   */


    /* Setup trap function */
    asm( "move.l #0x13, %d0" );
    asm( "trap #15" );

    /* Restore registers  */
    asm(" move.l (%a7)+, %d1" );
    asm(" move.l (%a7)+, %d0" );
}


/**
 * @brief: Prints a C-style null terminated string
 * @param: s the string to output to janusROM terminal 
 */
SINT32 rtx_dbug_outs( CHAR* s )
{
    if ( s == NULL )
    {
        return RTX_ERROR;
    }
    while ( *s != '\0' )
    {
        rtx_dbug_out_char( *s++ );
    }
    return RTX_SUCCESS;
}

/* print a signed integer */
SINT32 rtx_dbug_out_number(UINT32 number)
{
if (number ==0){
  rtx_dbug_outs("0\r\n");
}
  UINT32 temp = number;
  int counter = 0;
  while (temp > 0)
  {
    temp /= 10;
    counter ++;
  }
  char string[counter + 3];
  string[counter + 2] = '\0';
  string[counter + 1] = '\n';
  string[counter] = '\r';

  temp = number;

  while (counter > 0)
  {
    counter --;
    int t = temp %10;
    string[counter] = t + '0';
    temp /= 10;
  }

  rtx_dbug_outs(string);
  return RTX_SUCCESS;
}
