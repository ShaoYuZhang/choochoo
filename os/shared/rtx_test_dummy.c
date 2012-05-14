/*--------------------------------------------------------------------------
 *                      RTX Test Suite 
 *--------------------------------------------------------------------------
 */
/**

case8: receive, then send: lower send, higher receive
process 2 receive message. gets blocked.
process 1 send message to process 2. 
	re-empted. process 2 unblocked.
process 2 receive message. gets blocked.
process 1 send message to process 2. 
	re-empted. process 2 unblocked.
...
repeat until process one gets blocked by lack of memory
processes 3, 4, 5, 6 releases processor
no release_memory is called

 */

#include "rtx_test.h"
#include "dbug.h"

/* third party dummy test process 1 */ 
void test1()
{
    
    while (1) 
    {

    	rtx_dbug_outs((CHAR *)"rtx_test: test1\r\n");

        /* execute a rtx primitive to test */
		void * message = g_test_fixture.request_memory_block();
    g_test_fixture.delayed_send(2, message, 10);
	
	int sender_ID;
	void * data = g_test_fixture.receive_message(&sender_ID);
		g_test_fixture.release_processor();
	}
}

/* third party dummy test process 2 */ 
void test2()
{
    while (1) 
    {
    	//rtx_dbug_outs((CHAR *)"rtx_test: test2\r\n");


        /* execute a rtx primitive to test */
		//int sender_ID;
		//void * data = g_test_fixture.receive_message(&sender_ID);
		//g_test_fixture.release_memory_block(data);
	int sender_ID;
	rtx_dbug_outs("Trying to receive=======================================================================");
	void * data = g_test_fixture.receive_message(&sender_ID);
	rtx_dbug_outs("Received=======================================================================");
		g_test_fixture.release_processor();
	}
}
/* third party dummy test process 3 */ 
void test3()
{
    while (1) 
    {
	rtx_dbug_outs((CHAR *)"rtx_test: test3\r\n");


        /* execute a rtx primitive to test */
        //g_test_fixture.receive_message(4);
			int sender_ID;
	void * data = g_test_fixture.receive_message(&sender_ID);
		g_test_fixture.release_processor();
    }
}

/* third party dummy test process 4 */ 
void test4()
{
    while (1) 
    {
	rtx_dbug_outs((CHAR *)"rtx_test: test4\r\n");


        /* execute a rtx primitive to test */
			int sender_ID;
	void * data = g_test_fixture.receive_message(&sender_ID);
        g_test_fixture.release_processor();
    }
}
/* third party dummy test process 5 */ 
void test5()
{
    while (1) 
    {
	rtx_dbug_outs((CHAR *)"rtx_test: test5\r\n");


        /* execute a rtx primitive to test */
			int sender_ID;
	void * data = g_test_fixture.receive_message(&sender_ID);
        g_test_fixture.release_processor();
    }
}
/* third party dummy test process 6 */ 
void test6()
{
    while (1) 
    {
	rtx_dbug_outs((CHAR *)"rtx_test: test6\r\n");


        /* execute a rtx primitive to test */
			int sender_ID;
	void * data = g_test_fixture.receive_message(&sender_ID);
        g_test_fixture.release_processor();
    }
}

/* register the third party test processes with RTX */
void __attribute__ ((section ("__REGISTER_TEST_PROCS__")))register_test_proc()
{
    int i;

 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"rtx_test: register_test_proc()\r\n");
#endif


    for (i =0; i< NUM_TEST_PROCS; i++ ) {
        g_test_proc[i].pid = i + 1;
        g_test_proc[i].priority = 3;
        g_test_proc[i].sz_stack = 2048;
    }

	g_test_proc[0].priority = 2;
	g_test_proc[1].priority = 2;
    g_test_proc[0].entry = test1;
    g_test_proc[1].entry = test2;
    g_test_proc[2].entry = test3;
    g_test_proc[3].entry = test4;
    g_test_proc[4].entry = test5;
    g_test_proc[5].entry = test6;
}

/**
 * Main entry point for this program.
 * never get invoked
 */
int main(void)
{
 #ifdef _DEBUG
    rtx_dbug_outs((CHAR *)"rtx_test: started\r\n");
#endif

    return 0;
}
