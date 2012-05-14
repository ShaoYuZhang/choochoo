// ---------------------------------------------------------------------------
//                       RTX Test Suite
// --------------------------------------------------------------------------
// TODO test changing priority of blocked process.

#include "../shared/rtx_test.h"
#include "../shared/dbug.h"
#include "../shared/process_priority.h"
#include "test_inc.h"

enum TEST_PROCESS_INIT_PRIORITY
{
  T1_iPRIORITY = HIGHEST,
  T2_iPRIORITY = HIGH,
  T3_iPRIORITY = MEDIUM,
  T4_iPRIORITY = LOW,
  T5_iPRIORITY = LOWEST,
  T6_iPRIORITY = HIGHEST
};

/* third party dummy test process 1 */
void test1()
{
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"rtx_test: test1\r\n");
#endif


  // Test whether release_processor gets back to self (when self is highest)
  {
    void* haha = g_test_fixture.request_memory_block();
    int r =  g_test_fixture.send_message(T1_PID, haha);
    RECORD(r);

    g_test_fixture.release_processor();

    int sender;
    void* message = g_test_fixture.receive_message(&sender);
    r = (sender == T1_PID);
    RECORD(r);
    r = g_test_fixture.release_memory_block(message);
    RECORD(r);
  }

  // Delay send to self to prevent potential deadlock.
  {
    void* message = g_test_fixture.request_memory_block();
    int r =  g_test_fixture.delayed_send(T1_PID, message, 2000);

    RECORD(r);
  }

  // change our own priority, and make another other run.
  {
    int r = (g_test_fixture.set_process_priority(T1_PID, LOW) != 0) ? 1 : 0;
    // Verified via get_process_priority.
  }

  // When i wake up because of a change of priority,
  // everyone else should have same priority of LOWEST.
  {
    //int sender;
    //void* message = g_test_fixture.receive_message(&sender);
    //message
  }

  ALWAYS_RELEASE(T1_PID);
}

/* third party dummy test process 2 */
void test2()
{
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"rtx_test: test2\r\n");
#endif


  // Send msg to P1
  {
    void* msg = g_test_fixture.request_memory_block();
    int r = g_test_fixture.send_message(T1_PID, msg);
    RECORD(r);
  }

  // Check P1's priority is low.
  {
    int p1_priority = g_test_fixture.get_process_priority(T1_PID);
    int r = (p1_priority == LOW);
    RECORD(r);
  }

  // Test whether we change our own priority, and make another other run.
  {
    // Change, Make p2 run.
    int r = (g_test_fixture.set_process_priority(T2_PID, LOWEST) != 0) ? 1 : 0;
    RECORD(r);
  }

  // done test.
  ALWAYS_RELEASE(T2_PID);
}

/* third party dummy test procesos_tests.lsts 3 */
void test3()
{
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"rtx_test: test3\r\n");
#endif


  // Send msg to P1
  {
    void* msg = g_test_fixture.request_memory_block();
    int r = g_test_fixture.send_message(T1_PID, msg);
    RECORD(r);
  }

  // Check P2's priority is lowest.
  {
    int p2_priority = g_test_fixture.get_process_priority(T1_PID);
    int r = (p2_priority == LOWEST);
    RECORD(r);
  }

  // Tell P1 to wake up .. later.
  {
    void* message = g_test_fixture.request_memory_block();
    int r =  g_test_fixture.delayed_send(T1_PID, message, 10);
    RECORD(r);
  }

  // Test whether we change our own priority, and make another other run.
  {
    // Change, make p2 run.
    int r = (g_test_fixture.set_process_priority(T3_PID, LOWEST) != 0) ? 1 : 0;
    RECORD(r);
  }

  ALWAYS_RELEASE(T3_PID);
}

/* third party dummy test process 4 */
void test4()
{
  ALWAYS_RELEASE(T4_PID);
}

/* third party dummy test process 5 */
void test5()
{
  ALWAYS_RELEASE(T5_PID);
}

#include "score_keeper.c"

// register the third party test processes with RTX
void __attribute__ ((section ("__REGISTER_TEST_PROCS__")))register_test_proc()
{
 #ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"rtx_test: register_test_proc()\r\n");
#endif


  int i;
  for (i =0; i< NUM_TEST_PROCS; i++) {
    g_test_proc[i].pid = i + 1;
    g_test_proc[i].sz_stack = 2048;
  }

  // Set the initial priorities.
  g_test_proc[0].priority = T1_iPRIORITY;
  g_test_proc[1].priority = T2_iPRIORITY;
  g_test_proc[2].priority = T3_iPRIORITY;
  g_test_proc[3].priority = T4_iPRIORITY;
  g_test_proc[4].priority = T5_iPRIORITY;
  g_test_proc[5].priority = T6_iPRIORITY;

  // Set the start addresses for processes.
  g_test_proc[0].entry = test1;
  g_test_proc[1].entry = test2;
  g_test_proc[2].entry = test3;
  g_test_proc[3].entry = test4;
  g_test_proc[4].entry = test5;
  g_test_proc[5].entry = score_keeper;
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
