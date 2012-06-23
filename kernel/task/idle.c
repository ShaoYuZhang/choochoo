#include <idle.h>
#include <util.h>
#include <syscall.h>
#include <UserInterface.h>
#include <NameServer.h>
#include <IoServer.h>

extern int CALIBRATION;

void worker() {
  char uiName[] = UI_TASK_NAME;
  int ui = WhoIs(uiName);

  while(1) {
    int percentage;
    int tid = -1;
    Receive(&tid, (char*)&percentage, 4);
    UiMsg msg;
    msg.type = UPDATE_IDLE;
    msg.data3 = percentage;
    Send(ui, (char*)&msg, sizeof(UiMsg), (char*)1, 0);
    Reply(tid, (char*)1, 0);
  }
}

void idle() {
  unsigned int lastReportTime = GET_TIMER4();
  unsigned int idleTimeStart = 0;
  unsigned int totalIdleTime = 0;
  int createdWorker = 0;

  while (1) {
    //volatile unsigned int* currentTimePtr = 0x80810060;
    //unsigned int currentTime = *currentTimePtr;
    //totalIdleTime += currentTime - idleTimeStart;

    //if ((currentTime - lastReportTime) > 1000000 && !CALIBRATION) {
      //if (!createdWorker) {
      //  createdWorker = Create(1, worker);
      //}

      char com2name[] = IOSERVERCOM2_NAME;
      int com2 = WhoIs(com2name);
      bwputc(COM2, 'a');
      bwputc(COM2, 'a');
      bwputc(COM2, 'a');
      bwputc(COM2, 'a');
      bwputc(COM2, 'a');
    //  int percentage = 100;
     // UiMsg msg;
     // msg.type = UPDATE_IDLE;
     // msg.data3 = percentage;
     // Send(ui, (char*)&msg, sizeof(UiMsg), (char*)1, 0);

      //(totalIdleTime*100)/ ((currentTime - lastReportTime)*100);
      //Send(createdWorker, (char*)&percentage, 4, (char*)1,0);

    //  lastReportTime = currentTime;
    //  totalIdleTime = 0;
    //}
  }
}

int startIdleTask() {
  return Create(LOWEST_PRIORITY, idle);
}
