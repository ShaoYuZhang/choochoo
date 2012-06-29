#include <CommandDecoder.h>
#include <Driver.h>
#include <Track.h>
#include <IoServer.h>
#include <NameServer.h>
#include <UserInterface.h>
#include <syscall.h>
#include <IoHelper.h> // need a2i
#include <kernel.h>


static char decoderBuffer[DECODER_BUFFER_SIZE];
static unsigned int decoderCurrBufferPos;

static int trainController;
static int trackController;
static int com2;

static void decodeCommand() {
  decoderBuffer[decoderCurrBufferPos] = 0;
  if (decoderCurrBufferPos == 1 && decoderBuffer[0] == 'q') {
    kernel_quit();
  }
  unsigned int shortEvalulation = (decoderCurrBufferPos <= 3);
  decoderCurrBufferPos = 0;
  if (shortEvalulation) return;

  if (decoderBuffer[0] == 't' && decoderBuffer[1] == 'r') {
    int train_number = 0;
    int train_speed;
    char *temp = (char *)decoderBuffer + 3;
    char c = *temp++;
    c = a2i(c, &temp, 10, &train_number);
    c = *temp++;
    c = a2i(c, &temp, 10, &train_speed);

    DriverMsg msg;
    msg.type = SET_SPEED;
    msg.trainNum = train_number;
    msg.data2 = train_speed;
    Send(trainController, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
  } else if (decoderBuffer[0] == 'r' && decoderBuffer[1] == 'v') {
    int train_number = 0;
    char *temp = (char *)decoderBuffer + 3;
    char c = *temp++;
    c = a2i(c, &temp, 10, &train_number);

    DriverMsg msg;
    msg.type = SET_SPEED;
    msg.trainNum = train_number;
    msg.data2 = -1;
    Send(trainController, (char *)&msg, sizeof(DriverMsg), (char *)NULL, 0);
  } else if (decoderBuffer[0] == 's' && decoderBuffer[1] == 'w') {
    int switch_number = 0;
    char switch_pos;
    char *temp = (char *)decoderBuffer + 3;
    char c = *temp++;
    c = a2i(c, &temp, 10, &switch_number);
    switch_pos = *temp++;
    if (switch_pos == 's' || switch_pos == 'c') {
      TrackMsg setSwitch;

      TrackLandmark sw;
      sw.type = LANDMARK_SWITCH;
      sw.num1 = 0;
      sw.num2 = (char)switch_number;

      setSwitch.type = SET_SWITCH;
      setSwitch.landmark1 = sw;
      setSwitch.data = switch_pos == 'c' ? SWITCH_CURVED : SWITCH_STRAIGHT;
      Send(trackController, (char*)&setSwitch, sizeof(TrackMsg), (char *)NULL, 0);
    }
  }
}

static void commandDecoder() {
  decoderCurrBufferPos = 0;
  char com2Name[] = IOSERVERCOM2_NAME;
  com2 = WhoIs(com2Name);
  char trainControllerName[] = TRAIN_CONTROLLER_NAME;
  trainController = WhoIs(trainControllerName);
  char trackControllerName[] = TRACK_NAME;
  trackController = WhoIs(trackControllerName);
  char uiName[] = UI_TASK_NAME;
  int ui = WhoIs(uiName);

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
        decoderBuffer[decoderCurrBufferPos] = c;
        ++decoderCurrBufferPos;
        msg.data2 = decoderCurrBufferPos;
        Send(ui, (char*)&msg, sizeof(UiMsg), (char*)1, 0);
      }
    }
  }
}

int startCommandDecoderTask() {
  return Create(10, commandDecoder);
}
