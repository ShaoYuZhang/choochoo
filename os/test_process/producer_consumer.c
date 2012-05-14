// ---------------------------------------------------------------------------
//                       RTX Test Suite
// --------------------------------------------------------------------------

#include "../shared/rtx_test.h"
#include "../shared/dbug.h"
#include "../shared/process_priority.h"
#include "test_inc.h"

// Initial priorities ONLY, subject to change.
enum TEST_PROCESS_INIT_PRIORITY
{
  T1_iPRIORITY = HIGH,
  T2_iPRIORITY = HIGH,
  T3_iPRIORITY = HIGH,
  T4_iPRIORITY = HIGH,
  T5_iPRIORITY = LOW,
  T6_iPRIORITY = HIGHEST
};

int rand [100] = {0,2,5,3,4,3,5,3,0,4,5,0,0,1,
                  3,5,5,0,1,3,3,0,0,4,5,1,2,1,
                  4,4,4,4,4,3,3,4,0,3,3,5,4,4,
                  3,0,3,1,1,5,3,0,5,4,5,0,4,4,
                  3,3,0,1,2,0,2,0,2,4,0,4,4,3,
                  0,5,2,0,1,2,1,3,5,5,1,1,3,5,
                  3,0,2,0,1,3,4,3,0,0,1,0,0,5,4,5};

// ---------------------------------------------------------------------------
/* third party dummy test process 1 */
void producer1()
{
#ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"rtx_test: test1\r\n");
#endif

#if 0
  int seed = 34;

  // Now begin producing.
  while(1)
  {
    int remainingMsg = rand[(seed++)%100];
    for (; remainingMsg > 0; remainingMsg--)
    {
      void* envelop = g_test_fixture.request_memory_block();
      char* msg = ((char*)envelop) + 64;
      int i;
      for (i = 0; i < 64; i++){
        msg[i] = '1';
      }
      g_test_fixture.send_message(T4_PID, envelop);
    }

    // Let others produce a bit.
    g_test_fixture.release_processor();
  }
 #endif
 
  ALWAYS_RELEASE(T1_PID);
}

// ---------------------------------------------------------------------------
/* third party dummy test process 2 */
void producer2()
{
#ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"rtx_test: test2\r\n");
#endif
// Now begin producing.

  // int seed = 78;

  
  // while(1)
  // {
    // int remainingMsg = rand[(seed++)%100];
    // for (; remainingMsg > 0; remainingMsg--)
    // {
      // void* envelop = g_test_fixture.request_memory_block();
      // char* msg = ((char*)envelop) + 64;
      // int i;
      // for (i = 0; i < 64; i++){
        // msg[i] = '2';
      // }
      // g_test_fixture.send_message(T4_PID, envelop);
    // }

    // g_test_fixture.release_processor();
  // }
  
  ALWAYS_RELEASE(T2_PID);
}

// ---------------------------------------------------------------------------
/* third party dummy test procesos_tests.lsts 3 */
void producer3()
{
#ifdef _DEBUG
  rtx_dbug_outs((CHAR *)"rtx_test: test3\r\n");
#endif

/*   int seed = 12;

  // Now begin producing.
  while(1)
  {
    int remainingMsg = rand[(seed++)%100];
    for (; remainingMsg > 0; remainingMsg--)
    {
      void* envelop = g_test_fixture.request_memory_block();
      char* msg = ((char*)envelop) + 64;
      int i;
      for (i = 0; i < 64; i++){
        msg[i] = '3';
      }
      g_test_fixture.send_message(T4_PID, envelop);
    }

    // Let others produce a bit.
    g_test_fixture.release_processor();
  } */
  
  ALWAYS_RELEASE(T3_PID);
}

// ---------------------------------------------------------------------------
/* Consumer process */
void consumer()
{
  /* int numMsgReceived = 0;
  BOOLEAN dataIntegritySuccess = TRUE;
  void* envelop;

  while (TRUE)
  {
    int senderId;
    envelop = g_test_fixture.receive_message(&senderId);
    numMsgReceived += 1;

    char* msg = ((char*)envelop) + 64;

    // Flag for checking the checking are all valid.
    char beginChar = msg[0];
    int cur;
    for(cur= 0; cur < 64 && dataIntegritySuccess; cur++)
    {
      if (msg[cur] != beginChar) {
        dataIntegritySuccess = FALSE;
      }
    }

    if (numMsgReceived >= 1000)
    {
      // Test finished.

      // Record if data in messages were consistent.
      RECORD(dataIntegritySuccess);
	 
	  
      break;
    }

    g_test_fixture.release_memory_block(envelop);
  }
 */
#ifdef PART2
  g_test_fixture.set_process_priority(T1_PID, LOW);
  g_test_fixture.set_process_priority(T2_PID, HIGH);
  g_test_fixture.set_process_priority(T3_PID, MEDIUM);
  g_test_fixture.set_process_priority(T4_PID, HIGHEST);

  g_test_fixture.set_process_priority(T5_PID, LOWEST);

  // Delay send to self to prevent potential deadlock.
  {
    void* message = g_test_fixture.request_memory_block();
    int i;
    int r;
    int id;
    for (i = 0; i < 100; i++){
      r |= g_test_fixture.delayed_send(T4_PID, message, 1);
      message = g_test_fixture.receive_message(&id);
    }

    RECORD(r);
  }

  // change our own priority, and make another other run.
  {
    int r = 0;
    r |= (g_test_fixture.get_process_priority(T1_PID) != LOW)     ? 1 : 0;
    r |= (g_test_fixture.set_process_priority(T2_PID) != HIGH)    ? 1 : 0;
    r |= (g_test_fixture.set_process_priority(T2_PID) != MEDIUM)  ? 1 : 0;
    r |= (g_test_fixture.set_process_priority(T2_PID) != HIGHEST) ? 1 : 0;
    RECORD(r);
  }
// int* message_type = (int*)msg;
	  // *message_type = STOP_MSG_TYPE;
	  // g_test_fixture.send_message(T6_PID, envelop);
#endif

  ALWAYS_RELEASE(T4_PID);
}

// ---------------------------------------------------------------------------
/* third party dummy test process 5 */
void test5() { ALWAYS_RELEASE(T5_PID); }

// ---------------------------------------------------------------------------

char PASS_MSG[]  = "G07_test: test 00 PASS";
char FAIL_MSG[] = "G07_test: test 00 FAIL";

/* third party dummy test process 6 */
void score_keeper()
{
  rtx_dbug_outs((char*) "G07_test: START");

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
    r = (g_test_fixture.release_memory_block(message) == 0);
    RECORD(r);
  }

  // Test whether the priority is set correctly.
  {
    int r = (T1_iPRIORITY == g_test_fixture.get_process_priority(T1_PID));
    r += (T2_iPRIORITY == g_test_fixture.get_process_priority(T2_PID));
    r += (T3_iPRIORITY == g_test_fixture.get_process_priority(T3_PID));
    r += (T4_iPRIORITY == g_test_fixture.get_process_priority(T4_PID));
    r += (T5_iPRIORITY == g_test_fixture.get_process_priority(T5_PID));
    r += (T6_iPRIORITY == g_test_fixture.get_process_priority(T6_PID));
    r = (r == 6);
    RECORD(r);
  }

  // Send message to self if deadlocked later..
  {
    void* message = g_test_fixture.request_memory_block();
    int r =  g_test_fixture.delayed_send(T6_PID, message, 10000);
    RECORD(r);
  }

  // Prep T1 to run.
  {
    // Upgrade TEST1 to highest.
    int r = g_test_fixture.set_process_priority(T1_PID, HIGHEST);
    RECORD(r);
  }

  int numTestsPassed = 0;
  int totalNumberOfTests = 0;
  // Record.. will eventually unblock.. due to msg to self.
  while(1)
  {
    int senderId;
    char* envelop = (char*)g_test_fixture.receive_message(&senderId);
    TestData* data = (TestData*)(envelop + 64);

    if (data->m_messageType == TEST_RESULT_MSG_TYPE)
    {
      char* msg;
      if (data->m_data == 0){
        msg = PASS_MSG;
        numTestsPassed += 1;
      }
      else {
        msg = FAIL_MSG;
      }

      msg[15] = '0' + totalNumberOfTests/10;
      msg[16] = '0' + totalNumberOfTests%10;
      rtx_dbug_outs(msg);
    }
    else if (data->m_messageType == STOP_MSG_TYPE){
      break;
    }
    else {
      numTestsPassed = -2;
      break;
    }

    // Release the memory
    if (g_test_fixture.release_memory_block((void*)envelop)) {
      numTestsPassed = -1;
      break;
    }
  }

  // Interpret numTestsPassed.
  switch (numTestsPassed) {
    case -1:
    case -2:
      {
        rtx_dbug_outs((char*) "G07_test: Abrupt end to tests.");
      }
      default:
      {
	    
        rtx_dbug_outs((char*) "G07_test:  tests PASS");
        break;
      }
  } // End switch

  rtx_dbug_outs((char*) "G07_test: END");
  ALWAYS_RELEASE(T6_PID);
}



// ---------------------------------------------------------------------------
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
  g_test_proc[0].entry = producer1;
  g_test_proc[1].entry = producer2;
  g_test_proc[2].entry = producer3;
  g_test_proc[3].entry = consumer;
  g_test_proc[4].entry = test5; // unused.
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
