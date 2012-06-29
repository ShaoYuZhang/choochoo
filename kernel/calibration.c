#include <bwio.h>
#include <kernel.h>
#include <syscall.h>
#include <memory.h>
#include <NameServer.h>
#include <TimeServer.h>
#include <Driver.h>
#include <Track.h>
#include <CommandDecoder.h>
#include <IoServer.h>
#include <UserInterface.h>
#include <Sensor.h>
#include <CalibrationTask.h>

int CALIBRATION = 1;

void task1() {
  startNameServerTask();
  int id = startTimeServerTask();
  startIoServerTask();
  startSensorServerTask();
  startUserInterfaceTask();
  startDriverControllerTask();
  startTrackManagerTask();
  startCommandDecoderTask();
  startCalibrationTask();

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
