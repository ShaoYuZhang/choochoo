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

#if 0
  DriverMsg msg;
  msg.trainNum = 44;
  msg.type = SET_ROUTE;
  msg.data2 = 6;

  Position pos1;
  pos1.landmark1.type = LANDMARK_END;
  pos1.landmark1.num1 = EN;
  pos1.landmark1.num2 = 5;
  pos1.landmark2.type = LANDMARK_SENSOR;
  pos1.landmark2.num1 = 0;
  pos1.landmark2.num2 = 1;
  pos1.offset = 400;

  Position pos2;
  pos2.landmark1.type = LANDMARK_SENSOR;
  pos2.landmark1.num1 = 0;  // 2  0 0
  pos2.landmark1.num2 = 7;  // 3 14 9
  pos2.landmark2.type = LANDMARK_END;
  pos2.landmark2.num1 = EX; //
  pos2.landmark2.num2 = 10;  // 4 3  7
  pos2.offset = 300;
#endif

#if 0
  Position pos2;
  pos2.landmark1.type = LANDMARK_SENSOR;
  pos2.landmark1.num1 = 4;
  pos2.landmark1.num2 = 7;
  pos2.landmark2.type = LANDMARK_SENSOR;
  pos2.landmark2.num1 = 3;
  pos2.landmark2.num2 = 7;
  pos2.offset = 300;

  msg.pos1 = pos1;
  msg.pos2 = pos2;
  Send(trainController, (char*)&msg, sizeof(DriverMsg), (char*)1, 0);
#endif

#if 0
  TrackMsg trackmsg;
  trackmsg.type = ROUTE_PLANNING;
  trackmsg.position1 = pos1;
  trackmsg.position2 = pos2;

  Route route;
  Send(trackManager, (char*)&trackmsg, sizeof(TrackMsg), (char*)&route, sizeof(Route));

  PrintDebug(uiServer, "Distance %d ???\n", route.dist);
  PrintDebug(uiServer, "Num Node %d ???\n", route.length);

  for (int i = 0; i < route.length; i++) {
    RouteNode node = route.nodes[i];
    if (node.num == REVERSE) {
      PrintDebug(uiServer, "reverse %d\n", node.dist);
    } else {
      PrintDebug(uiServer, "Landmark %d %d %d %d", node.landmark.type, node.landmark.num1, node.landmark.num2, node.dist);
      if (node.landmark.type == LANDMARK_SWITCH && node.landmark.num1 == BR) {
        PrintDebug(uiServer, "Switch %d ", node.num);
      }
    }
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
  int trainController = startDriverControllerTask();
  int trackController = startTrackManagerTask();
  startCommandDecoderTask();

  Delay(500, time);
  Create(20, test_track);

  // Testing
  TrackMsg msg;
  msg.type = SET_TRACK;
  msg.data = 'b';
  Send(trackController, (char *)&msg, sizeof(TrackMsg), (char *)1, 0);

  Position pos;
  pos.landmark1.type = LANDMARK_SENSOR;
  pos.landmark1.num1 = 0;
  pos.landmark1.num2 = 4;
  pos.landmark2.type = LANDMARK_SENSOR;
  pos.landmark2.num1 = 1;
  pos.landmark2.num2 = 16;
  pos.offset = 100;

  DriverMsg drive;
  drive.trainNum = 39; // train
  drive.type = SET_ROUTE;
  drive.data2 = 8; // speed
  drive.pos = pos;

  Send(trainController, (char *)&drive, sizeof(DriverMsg), (char *)NULL, 0);

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
