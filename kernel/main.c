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
  // Testing track
  char com2name[] = IOSERVERCOM2_NAME;
  int com2 = WhoIs(com2name);
  char sensorName[] = SENSOR_NAME;
  int sensorServer = WhoIs(sensorName);
  char trackName[] = TRACK_NAME;
  int trackManager = WhoIs(trackName);
  char timename[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timename);

  TrackLandmark landmark1;
  TrackLandmark landmark2;

  landmark1.type = LANDMARK_END;
  landmark1.num1 = EN;
  landmark1.num2 = 5;

  landmark2.type = LANDMARK_END;
  landmark2.num1 = EX;
  landmark2.num2 = 3;

  TrackMsg trackmsg;
  trackmsg.type = ROUTE_PLANNING;
  trackmsg.landmark1 = landmark1;
  trackmsg.landmark2 = landmark2;

  Route route;
  Send(trackManager, (char*)&trackmsg, sizeof(TrackMsg), (char*)&route, sizeof(Route));

  printff(com2, "Distance %d \n", route.dist);
  printff(com2, "Num Node %d \n", route.length);

  for (int i = 0; i < route.length; i++) {
    RouteNode node = route.nodes[i];
    if (node.num == -1) {
      printff(com2, "reverse \n");
    } else {
      printff(com2, "Landmark %d %d %d ", node.landmark.type, node.landmark.num1, node.landmark.num2);
      if (node.landmark.type == LANDMARK_SWITCH && node.landmark.num1 == BR) {
        printff(com2, "Switch %d ", node.num);
      }

      printff(com2, "\n");
    }
  }
  Exit();
}

void task1() {
  startNameServerTask();
  startTimeServerTask();
  startIoServerTask();
  startSensorServerTask();
  startUserInterfaceTask();
  startDriverControllerTask();
  startTrackManagerTask();
  startCommandDecoderTask();

  // Testing
  //Create(20, test_track);

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
