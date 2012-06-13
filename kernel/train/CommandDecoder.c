#include <CommandDecoder.h>
#include <Train.h>
#include <IoServer.h>
#include <NameServer.h>
#include <syscall.h>
#include <bwio.h> // need a2i

static char decoderBuffer[DECODER_BUFFER_SIZE];
static unsigned int decoderCurrBufferPos;

static int trainController;
static int com2;

void decoderAddChar( char c );
void decodeCommand();

void commandDecoder() {
  decoderCurrBufferPos = 0;
  char com2Name[] = IOSERVERCOM2_NAME;
  com2 = WhoIs(com2Name);
  char trainControllerName[] = TRAIN_NAME;
  trainController = WhoIs(trainControllerName);

  for (;;) {
    char c = Getc(com2);
    if (c == '\r') {
      // TODO, clear line, UI stuff
      decodeCommand();
    } else {
      decoderAddChar( c );
    }
  }
}

void decoderAddChar( char c ) {
  if ( decoderCurrBufferPos > 0 && c == 8 ) {
    --decoderCurrBufferPos;
  } else if (decoderCurrBufferPos < DECODER_BUFFER_SIZE) {
    decoderBuffer[decoderCurrBufferPos] = c;
    ++decoderCurrBufferPos;
  }
}

void decodeCommand() {
  decoderBuffer[decoderCurrBufferPos] = 0;
  unsigned int temp_len = decoderCurrBufferPos;
  decoderCurrBufferPos = 0;
  TrainMsg msg;
  if (temp_len <= 3) {
    return;
  }

  if (decoderBuffer[0] == 't' && decoderBuffer[1] == 'r') {
    int train_number = 0;
    int train_speed;
    char *temp = (char *)decoderBuffer + 3;
    char c = *temp++;
    c = a2i(c, &temp, 10, &train_number);
    c = *temp++;
    c = a2i(c, &temp, 10, &train_speed);

    msg.type = SET_SPEED;
    msg.data1 = train_number;
    msg.data2 = train_speed;
    Send(trainController, (char *)&msg, sizeof(TrainMsg), (char *)NULL, 0);
  } else if (decoderBuffer[0] == 'r' && decoderBuffer[1] == 'v') {
    int train_number = 0;
    char *temp = (char *)decoderBuffer + 3;
    char c = *temp++;
    c = a2i(c, &temp, 10, &train_number);

    msg.type = SET_SPEED;
    msg.data1 = train_number;
    msg.data2 = -1;
    Send(trainController, (char *)&msg, sizeof(TrainMsg), (char *)NULL, 0);
  } else if (decoderBuffer[0] == 's' && decoderBuffer[1] == 'w') {
    int switch_number = 0;
    char switch_pos;
    char *temp = (char *)decoderBuffer + 3;
    char c = *temp++;
    c = a2i(c, &temp, 10, &switch_number);
    switch_pos = *temp++;
    if (switch_pos == 's' && switch_pos == 'c') {
      msg.type = SET_SWITCH;
      msg.data1 = switch_number;
      msg.data2 = switch_pos;
      Send(trainController, (char *)&msg, sizeof(TrainMsg), (char *)NULL, 0);
    }
  }
}


int startCommandDecoderTask() {
  return Create(10, commandDecoder);
}
