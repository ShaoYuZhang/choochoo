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
#include <RandomController.h>

int CALIBRATION;

void test_track() {
#if 0
  char trainControllerName[] = TRAIN_CONTROLLER_NAME;
  int trainController = WhoIs(trainControllerName);
  // Testing track
  char trackName[] = TRACK_NAME;
  int trackManager = WhoIs(trackName);
  char uiName[] = UI_TASK_NAME;
  int uiServer = WhoIs(uiName);

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
  int ui = startUserInterfaceTask();
  int trainController = startDriverControllerTask();
  int trackController = startTrackManagerTask();
  int randomController = startRandomTrainControllerTask();
  startCommandDecoderTask();

  //Create(20, test_track);

  // Testing
  TrackMsg msg;
  msg.type = SET_TRACK;
  msg.data = 'b';
  Send(trackController, (char *)&msg, sizeof(TrackMsg), (char *)1, 0);

#if 0
  Position pos;
  pos.landmark1.type = LANDMARK_SENSOR;
  pos.landmark1.num1 = 0;
  pos.landmark1.num2 = 2; //4;
  pos.landmark2.type = LANDMARK_END; //LANDMARK_SENSOR;
  pos.landmark2.num1 = EX; //1;
  pos.landmark2.num2 = 5; // 16;
  pos.offset = 100;

  DriverMsg drive;
  drive.trainNum = 39; // train
  drive.type = SET_ROUTE;
  drive.data2 = 8; // speed
  drive.pos = pos;

  Send(trainController, (char *)&drive, sizeof(DriverMsg), (char *)NULL, 0);
#endif

#if 0
    ReleaseOldAndReserveNewTrackMsg rmsg;
    rmsg.type = RELEASE_OLD_N_RESERVE_NEW;
    rmsg.trainNum = 44; // train num
    rmsg.stoppingDistance = 10;
    rmsg.numPredSensor = 0;

    char reply;
    rmsg.lastSensor.type = LANDMARK_SENSOR;
    rmsg.lastSensor.num1 = 2;
    rmsg.lastSensor.num2 = 14;
    rmsg.predSensor[0].type = LANDMARK_SENSOR;
    rmsg.predSensor[0].num1 = 0;
    rmsg.predSensor[0].num2 = 15;
    rmsg.predSensor[1].type = LANDMARK_SENSOR;
    rmsg.predSensor[1].num1 = 0;
    rmsg.predSensor[1].num2 = 4;
    rmsg.predSensor[2].type = LANDMARK_SENSOR;
    rmsg.predSensor[2].num1 = 0;
    rmsg.predSensor[2].num2 = 2;
    rmsg.predSensor[3].type = LANDMARK_SENSOR;
    rmsg.predSensor[3].num1 = 0;
    rmsg.predSensor[3].num2 = 14;
    rmsg.predSensor[4].type = LANDMARK_SENSOR;
    rmsg.predSensor[4].num1 = 0;
    rmsg.predSensor[4].num2 = 11;
    rmsg.numPredSensor = 5;
    rmsg.stoppingDistance = 485;

    Send(trackController, (char*)&rmsg, sizeof(ReleaseOldAndReserveNewTrackMsg), &reply, 1);
#endif

#if 0
    DriverMsg driveMsg;
    driveMsg.type = SET_ROUTE;
    driveMsg.data2 = 9; // speeed

    TrackMsg trackMsg;
    trackMsg.type = GET_RANDOM_POSITION;
    Send(trackController,
        (char*)&trackMsg, sizeof(TrackMsg), (char*)&driveMsg.pos,
        sizeof(Position));
    driveMsg.pos.offset = 0;

    PrintDebug(ui, "Random: Route %d %d %d %d %d %d %d %d %d",
        44, driveMsg.data2,
        driveMsg.pos.landmark1.type, driveMsg.pos.landmark1.num1,
        driveMsg.pos.landmark1.num2, driveMsg.pos.landmark2.type,
        driveMsg.pos.landmark2.num1, driveMsg.pos.landmark2.num2,
        driveMsg.pos.offset);

#endif


  Delay(600, time);

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
