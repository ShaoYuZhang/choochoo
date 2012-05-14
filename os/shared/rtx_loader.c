/*--------------------------------------------------------------------------
 *                      RTX and Test Suite Loader
 *--------------------------------------------------------------------------
 */
/**
 * @file:   rtx_loader.c   
 * @author: Thomas Reidemeister
 * @author: Irene Huang
 * @date:   2010.05.15
 * @brief:  To register third party test processes with rtx and load rtx 
 */

#include "rtx_test.h"
#include "dbug.h"

/* the RTX entry point, see rtx_loader.ld */
extern void __RTX_ENTRY__();    

/* rtx primitive registration function provided by the rtx 
 * rtx needs to register its primitives with test suite
 * The __REGISTER_RTX_ENTRY__ symbol is defined in 
 * linker scripts
 */

extern void __REGISTER_RTX_ENTRY__();

/**
 * Main entry point for this program.
 */
int main(void)
{
    
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"rtx_loader: started\r\n");
#endif


    /* register rtx primitives in the test data structure */
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"rtx_loader: calling __REGISTER_RTX_ENTRY__()...\r\n");
#endif

    __REGISTER_RTX_ENTRY__();

    /* start the rtx */
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"rtx_loader: calling __RTX_ENTRY__()...\r\n");
#endif

    __RTX_ENTRY__();


    /* should never reach here */
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"rtx_loader: out of rtx, not good!!!\r\n");
#endif

    while (1)
    {
 #ifdef _DEBUG
        rtx_dbug_outs((CHAR *)"rtx_loader: infinite loop!\r\n");
#endif

    }
    return 0;
}
