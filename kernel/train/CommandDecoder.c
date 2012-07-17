#include <CommandDecoder.h>
#include <Driver.h>
#include <IoHelper.h>
#include <RandomController.h>
#include <Track.h>
#include <IoServer.h>
#include <NameServer.h>
#include <UserInterface.h>
#include <syscall.h>
#include <IoHelper.h> // need a2i
#include <kernel.h>
#include <Track.h>
#include <Sensor.h>
#include <TimeServer.h>

static char decoderBuffer[DECODER_BUFFER_SIZE];
static unsigned int decoderCurrBufferPos;

static int randomController;
static int trainController;
static int trackController;
static int sensorServer;
static int timeServer;
static int com2;
static int ui;

// letter to A-E or X or N, doesn't support switches
static TrackLandmark formLandmarkFromInput(char letter, int number) {
  TrackLandmark landmark;
  landmark.type = LANDMARK_FAKE;
  landmark.num1 = 0;
  landmark.num2 = 0;
  if (letter >= 'a' && letter <= 'z') {
    letter = letter - 'a' + 'A';
  }

  if (letter >= 'A' && letter <= 'E') {
    landmark.type = LANDMARK_SENSOR;
    landmark.num1 = letter - 'A';
    landmark.num2 = number;
  } else if (letter == 'N') {
    landmark.type = LANDMARK_END;
    landmark.num1 = EN;
    landmark.num2 = number;
  } else if (letter == 'X') {
    landmark.type = LANDMARK_END;
    landmark.num1 = EX;
    landmark.num2 = number;
  } else {
    PrintDebug(ui, "No such landmark.");
  }
  return landmark;
}

static void decodeCommand() {
  decoderBuffer[decoderCurrBufferPos] = 0;
  if (decoderCurrBufferPos == 1 && decoderBuffer[0] == 'q') {
    kernel_quit();
  }
  unsigned int shortEvalulation = (decoderCurrBufferPos <= 1);
  decoderCurrBufferPos = 0;
  if (shortEvalulation) return;
  if (decoderBuffer[0] == 'i' && decoderBuffer[1] == 'n') {
    char *temp = (char *)decoderBuffer + 5;
    int train_number = strgetui(&temp);

    DriverMsg msg;
    msg.type = FIND_POSITION;
    msg.trainNum = train_number;
    Send(trainController, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
  } else if (decoderBuffer[0] == 't' && decoderBuffer[1] == 's') {
    char *temp = (char *)decoderBuffer + 3;

    char letter = *temp++;
    int num = strgetui(&temp);
    if (letter >= 'a' && letter <= 'e') {
      letter = letter - 'a' + 'A';
    }
    if (letter >= 'A' && letter <= 'E' && num >= 1 && num <= 16) {
      SensorMsg msg;
      msg.type = FAKE_TRIGGER;
      msg.box = letter - 'A';
      msg.data = (char)num;
      msg.time = Time(timeServer);
      Send(sensorServer, (char *)&msg, sizeof(SensorMsg), (char *)NULL, 0);
    } else {
      PrintDebug(ui, "Invalid Fake Sensor Value");
    }
  }  else if (decoderBuffer[0] == 'u' && decoderBuffer[1] == 's') {
    char *temp = (char *)decoderBuffer + 3;
    char track = *temp++;

    TrackMsg msg;
    msg.type = SET_TRACK;
    if (track == 'a' || track == 'b') {
      msg.data =track;
      Send(trackController, (char *)&msg, sizeof(TrackMsg), (char *)1, 0);
    }
  } else if (decoderBuffer[0] == 't' && decoderBuffer[1] == 'r') {
    char *temp = (char *)decoderBuffer + 3;
    int train_number = strgetui(&temp);
    temp++;
    int train_speed = strgetui(&temp);
    temp++;

    DriverMsg msg;
    msg.type = SET_SPEED;
    msg.trainNum = train_number;
    msg.data2 = train_speed;
    Send(trainController, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
  } else if (decoderBuffer[0] == 'r' && decoderBuffer[1] == 'v') {
    char *temp = (char *)decoderBuffer + 3;
    int train_number = strgetui(&temp);

    DriverMsg msg;
    msg.type = SET_SPEED;
    msg.trainNum = train_number;
    msg.data2 = -1;
    Send(trainController, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
  } else if (decoderBuffer[0] == 's' && decoderBuffer[1] == 'w') {
    char *temp = (char *)decoderBuffer + 3;
    int switch_number = strgetui(&temp);
    temp++;

    char switch_pos = *temp++;
    if (switch_pos == 's' || switch_pos == 'c') {
      TrackMsg setSwitch;

      TrackLandmark sw;
      sw.type = LANDMARK_SWITCH;
      sw.num1 = 0;
      sw.num2 = (char)switch_number;

      setSwitch.type = SUDO_SET_SWITCH;
      setSwitch.landmark1 = sw;
      setSwitch.data = switch_pos == 'c' ? SWITCH_CURVED : SWITCH_STRAIGHT;
      Send(trackController, (char*)&setSwitch, sizeof(TrackMsg), (char *)NULL, 0);

      DriverMsg trainMsg;
      trainMsg.trainNum = 255;
      trainMsg.type = BROADCAST_UPDATE_PREDICTION;
      Send(trainController, (char*)&trainMsg, sizeof(trainMsg), (char *)NULL, 0);
    }
  } else if (decoderBuffer[0] == 'r' && decoderBuffer[1] == 'o') {
    char *temp = (char *)decoderBuffer + 3;

    int train_number = strgetui(&temp);
    temp++;
    int train_speed = 9;
    if (*temp >= '0' && *temp <= '9'){
      train_speed = strgetui(&temp);
      temp++;
    }

    char letter;
    int num;

    Position pos;

    letter = *temp++;
    num = strgetui(&temp);
    temp++;
    pos.landmark1 = formLandmarkFromInput(letter, num);

    letter = *temp++;
    num = strgetui(&temp);
    temp++;
    pos.landmark2 = formLandmarkFromInput(letter, num);

    pos.offset = 0;
    if (*temp >= '0' && *temp <= '9'){
      pos.offset = strgetui(&temp);
    }

    DriverMsg msg;
    msg.trainNum = train_number;
    msg.type  = SET_ROUTE;
    msg.data2 = train_speed;
    msg.pos = pos;
    PrintDebug(ui, "CD: Route %d %d %d %d %d %d %d %d %d",
        train_number, train_speed,
        pos.landmark1.type, pos.landmark1.num1, pos.landmark1.num2,
        pos.landmark2.type, pos.landmark2.num1, pos.landmark2.num2,
        pos.offset);

    if (pos.landmark1.type == LANDMARK_FAKE ||
        pos.landmark2.type == LANDMARK_FAKE) {
      PrintDebug(ui, "Parse Fail: %s", decoderBuffer);
    } else {
      Send(trainController, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
    }
  } else if (decoderBuffer[0] == 'r' && decoderBuffer[1] == 'v') {
    char *temp = (char *)decoderBuffer + 3;

    int train_number = strgetui(&temp);
    temp++;
    int train_speed = 9;
    if (*temp >= '0' && *temp <= '9'){
      train_speed = strgetui(&temp);
      temp++;
    } else {
      PrintDebug(ui, "Parse fail: %s", decoderBuffer);
      return;
    }
    char letter;
    int num;


#if 0
    letter = *temp++;
    num = strgetui(&temp);
    temp++;
    Landmark land0 = formLandmarkFromInput(letter, num);

    letter = *temp++;
    num = strgetui(&temp);
    temp++;
    Landmark land1 = formLandmarkFromInput(letter, num);

    ReleaseOldAndReserveNewTrackMsg qMsg;
    qMsg.type = RELEASE_OLD_N_RESERVE_NEW;
    qMsg.trainNum = train_number;
    qMsg.stoppingDistance = 100;
#endif
  } else if (decoderBuffer[0] == 'r' && decoderBuffer[1] == 's') {
    char *temp = (char*)decoderBuffer + 3;

    int train_number = strgetui(&temp);
    TrackMsg msg;
    msg.type = QUERY_EDGES_RESERVED;
    msg.data = (char)train_number;
    Send(trackController, (char *)&msg, sizeof(TrackMsg), (char *)1, 0);
  } else if (decoderBuffer[0] == 'x' && decoderBuffer[1] == 'x') {
    char *temp = (char*)decoderBuffer + 3;
    int train_number = strgetui(&temp);
    Send(randomController, (char*)&train_number, 4, (char *)1, 0);
  } else if (decoderBuffer[0] == 't' && decoderBuffer[1] == 'm') {
    int train_number = 255;
    PrintDebug(ui, "Trains going in test mode");
    PrintDebug(ui, "Make sure smaller numbered train is at A4, larger numbered train at D6");
    DriverMsg msg;
    msg.type = BROADCAST_TEST_MODE;
    msg.trainNum = 255;
    msg.data2 = 8;
    Send(trainController, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
  } else {
    PrintDebug(ui, "Bad: %s", decoderBuffer);
  }
}

static void commandDecoder() {
  decoderCurrBufferPos = 0;
  char com2Name[] = IOSERVERCOM2_NAME;
  com2 = WhoIs(com2Name);
  char randomName[] = RANDOM_CONTROL_NAME;
  randomController = WhoIs(randomName);
  char trainControllerName[] = TRAIN_CONTROLLER_NAME;
  trainController = WhoIs(trainControllerName);
  char trackControllerName[] = TRACK_NAME;
  trackController = WhoIs(trackControllerName);
  char uiName[] = UI_TASK_NAME;
  ui = WhoIs(uiName);
  char sensorServerName[] = SENSOR_NAME;
  sensorServer = WhoIs(sensorServerName);
  char timeServerName[] = TIMESERVER_NAME;
  timeServer = WhoIs(timeServerName);

  UiMsg msg;
  msg.type = PROMPT_CHAR;
  for (;;) {
    char c = Getc(com2);
    msg.data3 = c;
    if (c == RETURN) {
      decodeCommand();
      msg.data2 = 1; // Move cursor to clear line.
      Send(ui, (char*)&msg, sizeof(UiMsg), (char*)1, 0);
    } else {
      if (decoderCurrBufferPos > 0 && c == BACKSPACE) {
        --decoderCurrBufferPos;
        msg.data2 = decoderCurrBufferPos+1;
        Send(ui, (char*)&msg, sizeof(UiMsg), (char*)1, 0);
      } else if (decoderCurrBufferPos < DECODER_BUFFER_SIZE) {
        if (( c >= '0' && c <= '9' ) ||
            ( c >= 'a' && c <= 'z' ) ||
            ( c >= 'A' && c <= 'Z' ) || c == ' ') {
          decoderBuffer[decoderCurrBufferPos] = c;
          ++decoderCurrBufferPos;
          msg.data2 = decoderCurrBufferPos;
          Send(ui, (char*)&msg, sizeof(UiMsg), (char*)1, 0);
        }
      }
    }
  }
}

int startCommandDecoderTask() {
  return Create(10, commandDecoder);
}
