#ifndef test_inc_h_
#define test_inc_h_

#define STOP_MSG_TYPE 0
#define TEST_RESULT_MSG_TYPE 1
typedef struct TestData
{
  int m_messageType;
  int m_data;
} TestData;

#define RECORD(success) \
{ \
  TestData* td = (TestData*)g_test_fixture.request_memory_block();  \
  td->m_messageType = TEST_RESULT_MSG_TYPE; \
  td->m_data = success; \
  g_test_fixture.send_message(T6_PID, td); \
} \

#define ALWAYS_RELEASE(pid) \
  g_test_fixture.set_process_priority(pid, LOW); \
  while (1) \
  { \
    g_test_fixture.release_processor();\
  }

enum TEST_PROCESS_PID
{
  T1_PID = 1,
  T2_PID,
  T3_PID,
  T4_PID,
  T5_PID,
  T6_PID
};

#endif // test_inc_h_
