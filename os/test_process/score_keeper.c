
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


