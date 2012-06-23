#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <Train.h>
#include <CommandDecoder.h>
#include <IoServer.h>
#include <UserInterface.h>
#include <Sensor.h>
#include <Track.h>
#include <IoHelper.h>

int CALIBRATION = 0;

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

  SensorMsg sensorMsg;
  sensorMsg.type = QUERY_RECENT;

  Sensor sensor;
  Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
      (char*)&sensor, sizeof(Sensor));
  TrackLandmark pastSensor;
  TrackLandmark currentSensor;

  int previousTime;
  int currentTime = Time(timeserver);

  currentSensor.type = LANDMARK_SENSOR;
  currentSensor.num1 = sensor.box;
  currentSensor.num2 = sensor.val;

  for (;;) {
    Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
        (char*)&sensor, sizeof(Sensor));
    pastSensor = currentSensor;
    currentSensor.type = LANDMARK_SENSOR;
    currentSensor.num1 = sensor.box;
    currentSensor.num2 = sensor.val;

    TrackMsg trackmsg;
    trackmsg.type = QUERY_DISTANCE;
    trackmsg.landmark1 = pastSensor;
    trackmsg.landmark2 = currentSensor;

    int dist;
    Send(trackManager, (char*)&trackmsg, sizeof(TrackMsg), (char*)&dist, 4);

    previousTime = currentTime;
    currentTime = Time(timeserver);
    int diff = currentTime - previousTime;

    printff(com2, "%d ",  dist * 100 / diff);
  }
  Exit();
}

void task1() {
  startNameServerTask();
  startTimeServerTask();
  startIoServerTask();
  startSensorServerTask();
  startUserInterfaceTask();
  startTrainControllerTask();
  startCommandDecoderTask();
  startTrackManagerTask();

  // Testing
  //Create(20, test_track);

  Exit();
}

int main(int argc, char* argv[]) {
	kernel_init();

  int returnVal;

  // Create task with low priority to ensure other initialized tasks are blocked.
  kernel_createtask((int*)&returnVal, 28, (int)task1, 0);

	kernel_runloop();
	return 0;
}
