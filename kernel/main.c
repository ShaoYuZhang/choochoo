#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <Driver.h>
#include <CommandDecoder.h>
#include <IoServer.h>
#include <UserInterface.h>
#include <Sensor.h>
#include <Track.h>
#include <IoHelper.h>

int CALIBRATION;

void test_track() {
  char trainControllerName[] = TRAIN_CONTROLLER_NAME;
  int trainController = WhoIs(trainControllerName);
  // Testing track
  char trackName[] = TRACK_NAME;
  int trackManager = WhoIs(trackName);
  char uiName[] = UI_TASK_NAME;
  int uiServer = WhoIs(uiName);
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);

  DriverMsg msg;
  msg.trainNum = 44;
  msg.type = SET_ROUTE;
  msg.data2 = 6;

  msg.landmark1.type = LANDMARK_END;
  msg.landmark1.num1 = EN;
  msg.landmark1.num2 = 5;

  msg.landmark2.type = LANDMARK_END;
  msg.landmark2.num1 = EX;
  msg.landmark2.num2 = 4;

  Send(trainController, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);

#if 0
  TrackMsg trackmsg;
  trackmsg.type = ROUTE_PLANNING;
  trackmsg.landmark1 = landmark1;
  trackmsg.landmark2 = landmark2;

  Route route;
  Send(trackManager, (char*)&trackmsg, sizeof(TrackMsg), (char*)&route, sizeof(Route));

  PrintDebug(uiServer, "Distance %d \n", route.dist);
  PrintDebug(uiServer, "Num Node %d \n", route.length);

  for (int i = 0; i < route.length; i++) {
    RouteNode node = route.nodes[i];
    if (node.num == REVERSE) {
      PrintDebug(uiServer, "reverse \n");
    } else {
      PrintDebug(uiServer, "Landmark %d %d %d %d", node.landmark.type, node.landmark.num1, node.landmark.num2, node.dist);
      if (node.landmark.type == LANDMARK_SWITCH && node.landmark.num1 == BR) {
        PrintDebug(uiServer, "Switch %d ", node.num);
      }
    }
    Delay(100, timeserver);
  }
#endif
  Exit();
}

void task1() {
  startNameServerTask();
  int time = startTimeServerTask();
  startIoServerTask();
  startSensorServerTask();
  startUserInterfaceTask();
  startDriverControllerTask();
  startTrackManagerTask();
  startCommandDecoderTask();


  Delay(700, time);
  // Testing
  Create(20, test_track);

  Exit();
}

int main(int argc, char* argv[]) {
  CALIBRATION = 0;
	kernel_init();

  int returnVal;

  // Create task with low priority to ensure other initialized tasks are blocked.
  kernel_createtask((int*)&returnVal, 28, (int)task1, 0);

	kernel_runloop();
	return 0;
}
