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

void task1() {
  startNameServerTask();
  int time = startTimeServerTask();
  startIoServerTask();
  startSensorServerTask();
  int ui = startUserInterfaceTask();
  int trainController = startDriverControllerTask();
  int trackController = startTrackManagerTask();
  int randomController = startRandomTrainControllerTask();
  startCommandDecoderTask();

  TrackMsg msg;
  msg.type = SET_TRACK;
  msg.data = 'b';
  Send(trackController, (char *)&msg, sizeof(TrackMsg), (char *)1, 0);

  PrintDebug(ui, "initing... waiting for other stuff to settle");
  Delay(500, time);
  startCalibrationTask();
  PrintDebug(ui, "Calibration");

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
