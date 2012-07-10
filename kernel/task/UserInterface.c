#include "UserInterface.h"
#include <util.h>
#include <NameServer.h>
#include <Driver.h>
#include <Track.h>
#include <TimeServer.h>
#include <IoServer.h>
#include <IoHelper.h>
#include <Sensor.h>
#include <CalibrationData.h>

#define REFRESH_TICK 100
#define NUM_SENSORSET 5
#define NUM_SENSOR_STATUS 21
#define UI_WIDTH 80
#define PROMPT_R1 '0'+2
#define PROMPT_R2 '0'+7
#define CLOCK_R1 '0'+2
#define CLOCK_R2 '0'+5
#define CLOCK_C1 '8'

static char* formatInt(int n, int numDigit, char* msg) {
  int isNegative = 0;
  if (n < 0) {
    isNegative = 1;
    n = -n;
  }
  int anyOutput = 0;
  int highestDigit = 0;
  for (int i = numDigit-1; i >= 0; i--) {
    if (n != 0){
      msg[i] = '0' + n%10;
      anyOutput = 1;
      highestDigit = i;
    } else {
      msg[i] = ' ';
    }

    n /= 10;
  }
  if (isNegative) {
    msg[highestDigit - 1] = '-';
  }
  if (!anyOutput) {
    msg[numDigit-1] = '0';
  }
  return msg + numDigit;
}

static char* resetColor(char* msg) {
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = '0';
  *msg++ = 'm';
  return msg;
}

static char* setColor(int color, char* msg) {
  *msg++ = ESC;
  *msg++ = '[';
  msg = formatInt(color, 2, msg);
  *msg++ = 'm';
  return msg;
}

static char* moveTo(char row, int col, char* msg) {
  // Move to position
  *msg++ = ESC;
  *msg++ = '[';
  if (row > 9) {
    *msg++ = '0' + row/10; // line
  }
  *msg++ = '0' + row%10; // line
  *msg++ = ';';

  if (col > 99) {
    *msg++ = '0' + col/100;
    col %= 100;
  }
  if (col > 9) {
    *msg++ = '0' + col/10; // co
  }
  *msg++ = '0' + col%10; // line
  *msg++ = 'f';
  return msg;
}

static char* saveCursor(char* msg) {
  // SaveCursor
  //*msg++ = ESC;
  //*msg++ = '[';
  //*msg++ = 's';

  return msg;
}

static char* restoreCursor(char* msg) {
  //*msg++ = ESC;
  //*msg++ = '[';
  //*msg++ = 'u';
  return msg;
}

static char* timeu(unsigned int ms, char* msg) {
  int min = (ms / 60000) % 60;
  int sec = (ms / 1000) % 60;
  int mso = (ms / 100) % 10;

  // Print the time
  *msg++ = min/10 + '0';
  *msg++ = min%10 + '0';
  *msg++ = ':';
  *msg++ = sec/10 + '0';
  *msg++ = sec%10 + '0';
  *msg++ = '.';
  *msg++ = mso + '0';

  return msg;
}
// .-------------+----------+
// |    Train    |  %d      |
// +-------------+----------|
// | L Sensor    |          |
// | L Sensor T  |          |
// | L Predic T  |          | <-- red if high
// |             |          |
// | Speed       |  2       |
// | Velocity    |  32mm/s  |
// | D From Last |          |
// |             |
// | N Sensor    |
// | N Sensor T  |
// | Location    | LMARK    |
// |             |   |      |
// |             |  256     |
// |             |   *      |
// |             |  25      |
// |             |   |      |
// |             | LMARK    |
static char* updateTrain(TrainUiMsg* train, char* msg) {
  msg = saveCursor(msg);

  msg = setColor(33+train->nth*3, msg);
  int col = 26+(int)(train->nth)*10;
  int row = 2;
  // TRAIN NUMBER
  msg = moveTo(row++, col, msg);
  *msg++ = 'N';
  *msg++ = 'u';
  *msg++ = 'm';
  *msg++ = ':';
  *msg++ = ' ';
  msg = formatInt(train->trainNum, 2, msg);
  row++;
  msg = resetColor(msg);

  // ---------------------------------
  // PAST INFORMATION
  if (train->lastSensorVal != 0){
    msg = moveTo(row++, col, msg);
    if (train->lastSensorUnexpected) msg = setColor(31, msg);
    if (train->lastSensorIsTerminal) {
      *msg++ = 'E';
      if (train->lastSensorBox == EX){
        *msg++ = 'X';
      } else if (train->lastSensorBox == EN){
        *msg++ = 'N';
      } else {
        *msg++ = 'W';
        *msg++ = 'W';
        *msg++ = 'W'; // =)
      }
    } else {
      *msg++ = 'A' + train->lastSensorBox;
    }
    msg = formatInt(train->lastSensorVal, 2, msg);
    *msg++ = ' ';
    *msg++ = ' ';

    msg = moveTo(row++, col, msg);
    msg = timeu(train->lastSensorActualTime, msg);
    if (train->lastSensorUnexpected) msg = resetColor(msg);
  } else {
    msg = moveTo(row++, col, msg);
    *msg++ = ' ';
    *msg++ = ' ';
    *msg++ = ' ';
    msg = moveTo(row++, col, msg);
    *msg++ = ' ';
    *msg++ = ' ';
    *msg++ = ' ';
    *msg++ = ' ';
    *msg++ = ' ';
    *msg++ = ' ';
    *msg++ = ' ';
  }

  row++;

  // ---------------------------------
  // PRESENT INFORMATION
  msg = moveTo(row++, col, msg);
  msg = formatInt(train->speed, 2, msg);

  if (train->speedDir == ACCELERATE) {
    *msg++ = '+'; *msg++ = '+';
  } else {
    *msg++ = '-'; *msg++ = '-';
  }

  msg = moveTo(row++, col, msg);
  msg = formatInt(train->velocity, 3, msg);

  msg = moveTo(row++, col, msg);
  msg = formatInt(train->distanceFromLastSensor, 6, msg);
  msg = moveTo(row++, col, msg);
  msg = formatInt(train->distanceToNextSensor, 6, msg);
  row++;

  // ---------------------------------
  // FUTURE INFORMATION
  msg = moveTo(row++, col, msg);
  if (train->nextSensorIsTerminal) {
    *msg++ = 'E';
    if (train->nextSensorBox == EX){
      *msg++ = 'X';
    } else if (train->nextSensorBox == EN){
      *msg++ = 'N';
    } else {
      *msg++ = 'W';
      *msg++ = 'W';
      *msg++ = 'W'; // =)
    }
  } else {
    *msg++ = 'A' + train->nextSensorBox;
  }
  msg = formatInt(train->nextSensorVal, 2, msg);
  *msg++ = ' ';
  *msg++ = ' ';

  row++;

  // ---------------------------------
  // ERROR INFORMATION
  msg = moveTo(row++, col, msg);
  msg = setColor(31, msg);
  msg = formatInt(train->lastSensorDistanceError, 6, msg);
  msg = resetColor(msg);
  return restoreCursor(msg);
}

static char* promptChar(char col, char c, char* msg) {
  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = PROMPT_R1;
  *msg++ = PROMPT_R2;
  *msg++ = ';';
  if (col / 100 != 0) {
    *msg++ = '1'; // Assume dont have greater than 100.
    col %= 100;
  }
  if (col / 10 != 0) {
    *msg++ = '0' + col/10;
  }
  *msg++ = '0'+col%10;
  *msg++ = 'f';

  // Print char
  if (c == BACKSPACE) {
    *msg++ = ' ';
    // Move cursor back
    *msg++ = ESC;
    *msg++ = '[';
    *msg++ = '1';
    *msg++ = 'D';
  }
  else if (c == RETURN) {
    *msg++ = ESC;
    *msg++ = '[';
    *msg++ = PROMPT_R1;
    *msg++ = PROMPT_R2;
    *msg++ = ';';
    *msg++ = '2';
    *msg++ = 'f';

    for (int i = 0; i < 48; i++){
      *msg++ = ' ';
    }
    *msg++ = ESC;
    *msg++ = '[';
    *msg++ = PROMPT_R1;
    *msg++ = PROMPT_R2;
    *msg++ = ';';
    *msg++ = '2';
    *msg++ = 'f';
  }
  else {
    *msg++ = c;
  }
  return msg;
}

static char* pad2(int n, char* msg){
  if (n/10) {
    *msg++ = '0' + n/10;
  }
  *msg++ = '0' + n%10;
  return msg;
}

static int debugUpdateNum;
static char* updateDebugMessage(char* receive, char* msg, int len) {
  msg = saveCursor(msg);

  int debugCol = (debugUpdateNum/50) % 3;
  // Actual position
  const int debugUpdateCol = 50 + 30 * debugCol;
  const int updateRow = debugUpdateNum%50 + 1;

  msg = moveTo(updateRow, debugUpdateCol, msg);

  // Write this line
  for (int i = 0; i < MAX(30, len); i++) {
    *msg++ = (i < len) ? receive[i] : ' ';
  }

  debugUpdateNum += 1;

  msg = restoreCursor(msg);
  return msg;
}

static char* updateTime(unsigned int ms, char* msg) {
  msg = saveCursor(msg);

  msg = moveTo(25, 8, msg);
  msg = timeu(ms, msg);

  return restoreCursor(msg);
}

static char* updateIdle(int percentage, char* msg) {
  msg = saveCursor(msg);

  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = CLOCK_R1;
  *msg++ = CLOCK_R2;
  *msg++ = ';';
  *msg++ = '1';
  *msg++ = 'f';

  // Print the idleness. ASSUME 4 digits. e.g. 99.55
  if (percentage > 999) {
    *msg++ = '0' + percentage / 1000;
    percentage /= 10;
  } else {
    *msg++ = '0';
  }

  if (percentage > 99) {
    *msg++ = '0' + percentage / 100;
    percentage /= 10;
  } else {
    *msg++ = '0';
  }
  *msg++ = '.';
  if (percentage > 9) {
    *msg++ = '0' + percentage / 10;
    percentage %= 10;
  } else {
    *msg++ = '0';
  }
  *msg++ = '0' + percentage;

  msg = restoreCursor(msg);

  return msg;
}

static int numUpdated;
static char* updateSensor(int box, int val, char* msg) {
  msg = saveCursor(msg);

  // move to position
  if (numUpdated == 0) {
    msg = moveTo(26, 10, msg);
    for (int i = 0; i < 40; i++) {
      *msg++ = ' ';
    }
    msg = moveTo(26, 10, msg);
  } else {
    msg = moveTo(26, 10 + numUpdated*5, msg);
  }

  // Print sensor info
  *msg++ = 'A' + box;
  if (val >= 100) {
    *msg++ = '0' + val/100;
    val /= 10;
    *msg++ = '0'+ val/10;
    *msg++ = '0'+ val%10;
  }
  else if (val >= 10) {
    *msg++ = '0'+ val/10;
    *msg++ = '0'+ val%10;
    *msg++ = ' ';
  }
  else {
    *msg++ = '0'+ val%10;
    *msg++ = ' ';
    *msg++ = ' ';
  }

  msg = restoreCursor(msg);

  numUpdated++;
  numUpdated &= 7; // Mod 8
  return msg;
}

static char* drawTrainFrameHelper(
    char* msg, char leftCorner, char center, char rightCorner,
    char* text1, char* text2) {
  *msg++ = leftCorner;
  for (int i = 0; i < 13; i++){
    if (text1 == NULL) {
      *msg++ = ' ';
    }
    else {
      *msg++ = text1[i];
    }
  }
  for(int j = 0; j < 2; j++){
    *msg++ = center;
    for (int i = 0; i < 9; i++){
      if (text2 == NULL) {
        *msg++ = ' ';
      }
      else {
        *msg++ = text2[i];
      }
    }
  }
  *msg++ = rightCorner;
  return msg;
}

static char* drawTrainFrame(char* msg) {
  int row = 1;
  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '.', '+', '+', "-------------", "----------");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', "    Train    ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '+', '+', '+', "-------------", "----------");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', " L Sensor    ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', " L Sensor T  ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', "             ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', " Speed       ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', " Velocity    ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', " D From Last ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', " D to Next   ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', "             ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', " N Sensor    ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', "             ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '|', '|', '|', " Miss by..mm ", "          ");

  msg = moveTo(row++, 10, msg);
  msg = drawTrainFrameHelper(msg, '`', '-', '`', "-------------", "----------");
  return msg;
}

static char* drawSwitches(char* msg) {
  msg = moveTo(1, 1, msg);

  // Text
  *msg++ = 'S';
  *msg++ = 'w';
  *msg++ = 'i';
  *msg++ = 't';
  *msg++ = 'c';
  *msg++ = 'h';

  for (int i = 1; i <= 18; i++) {
    *msg++ = '\r';
    *msg++ = '\n';

    // Switch info
    *msg++ = 'S';
    *msg++ = 'w';
    msg = pad2(i, msg);
    if (i < 10) {
      *msg++  = ' ';
    }
    *msg++  = ' ';
    *msg++  = ' ';
    *msg++  = ':';
    *msg++  = '~';
  }

  for (int i = 0; i < 4; i++) {
    *msg++ = '\r';
    *msg++ = '\n';
    // Text
    *msg++ = 'S';
    *msg++ = 'w';
    *msg++ = '0';
    *msg++ = 'x';
    *msg++ = '9';
    if (i == 0) {
      *msg++ = '9';
    } else if (i == 1) {
      *msg++ = 'a';
    } else if (i == 2) {
      *msg++ = 'b';
    } else if (i == 3) {
      *msg++ = 'c';
    }
    *msg++ = ':';
    *msg++ = '~';
  }

  return msg;
}

static char* updateSwitch(int sw, int ss, char* msg) {
  msg = saveCursor(msg);

  char state = ss == SWITCH_STRAIGHT ? '-' : '~';

  if (sw >= 0x99 && sw <= 0x9c) {
    sw -= 134; // make 0x99 19
  }
  sw ++;

  *msg++ = ESC;
  *msg++ = '[';
  if (sw > 9) {
    *msg++ = '0' + sw/10;
  }
  *msg++ = '0' + sw%10;
  *msg++ = ';';
  *msg++ = '8'; // Col
  *msg++ = 'f';

  *msg++ = state;

  msg = restoreCursor(msg);

  return msg;
}

static void timerDelay() {
  int parent = MyParentsTid();
  char timeName[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timeName);
  UiMsg msg;
  msg.type = UPDATE_TIME;
  int ticks = Time(timeserver);

  for(;;) {
    ticks += T100MS;
    DelayUntil(ticks, timeserver);
    msg.data3 = ticks*10; // 10ms per tick
    Send(parent, (char*)&msg, sizeof(UiMsg), (char*)1, 0);
  }
}

static void idleness() {
  int parent = MyParentsTid();
  char timeName[] = TIMESERVER_NAME;
  int timeserver = WhoIs(timeName);
  UiMsg msg;
  msg.type = UPDATE_IDLE;
  int ticks = Time(timeserver);

  for (;;) {
    ticks += 100; // 1 seconds
    DelayUntil(ticks, timeserver);
    Send(parent, (char*)&msg, sizeof(UiMsg), (char*)1, 0);
  }
}

static void sensorQuery() {
  int parent = MyParentsTid();
  char sensorName[] = SENSOR_NAME;
  int sensorServer = WhoIs(sensorName);
  UiMsg uimsg;
  uimsg.type = UPDATE_SENSOR;

  SensorMsg sensorMsg;
  sensorMsg.type = QUERY_RECENT;
  Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
      (char*)1, 0);
  for (;;) {
    Sensor sensor;
    int tid;
    Receive(&tid, (char *)&sensor, sizeof(Sensor));
    Reply(tid, (char *)1, 0);

    uimsg.data1 = sensor.box;
    uimsg.data2 = sensor.val;
    Send(parent, (char*)&uimsg, sizeof(UiMsg), (char*)1, 0);
  }
}

static void displayStaticContent(int com2) {
  char msgStart[2048];
  char* msg = msgStart;

  // Do not show cursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = '?';
  *msg++ = '2';
  *msg++ = '5';
  *msg++ = 'l';

  // Clear the screen
  *msg++ = ESC;
  *msg++ = 'c';
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = '2';
  *msg++ = 'J';

  msg = drawSwitches(msg);

  // Draw sensor
  msg = moveTo(26, 1, msg);

  // Text
  *msg++ = 'S';
  *msg++ = 'e';
  *msg++ = 'n';
  *msg++ = 's';
  *msg++ = 'o';
  *msg++ = 'r';
  *msg++ = ':';

  msg = drawTrainFrame(msg);
  msg = promptChar(1, '>', msg);

  Putstr(com2, msgStart, msg-msgStart);

}

static void userInterface() {
  char name[] = UI_TASK_NAME;
  RegisterAs(name);
  char com2name[] = IOSERVERCOM2_NAME;
  int com2 = WhoIs(com2name);
  Create(1, timerDelay);
  Create(1, sensorQuery);
  Create(10, idleness);

  displayStaticContent(com2);

  char com2msgStart[128];
  char receiveBuffer[128];
  UiMsg* msg = (UiMsg*)receiveBuffer;
  for (;;) {
    int tid = -1;

    char* com2msg = com2msgStart;
    int len = Receive(&tid, receiveBuffer, 128);
    Reply(tid, (char*)1, 0);

    switch (msg->type) {
      case PROMPT_CHAR: {
        com2msg = promptChar(msg->data2+1, msg->data3, com2msg);
        break;
      }
      case UPDATE_TIME: {
        com2msg = updateTime(msg->data3, com2msg);
        break;
      }
      case UPDATE_SENSOR: {
        com2msg = updateSensor(msg->data1 /*box*/, msg->data2 /*val*/, com2msg);
        break;
      }
      case UPDATE_SWITCH: {
        com2msg = updateSwitch(msg->data1 /*switch*/, msg->data2 /*state*/, com2msg);
        break;
      }
      case UPDATE_IDLE: {
        com2msg = updateIdle(Idleness(), com2msg);
        break;
      }
      case UPDATE_TRAIN: {
        com2msg = updateTrain((TrainUiMsg*)receiveBuffer, com2msg);
        break;
      }
      case DEBUG_TRAIN_MSG: {
        com2msg = setColor(33+receiveBuffer[1]*3, com2msg);
        com2msg = updateDebugMessage(receiveBuffer + 2, com2msg, len - 2);
        com2msg = resetColor(com2msg);
        break;
      }
      default: {
        com2msg = updateDebugMessage(receiveBuffer + 1, com2msg, len - 1);
      }
    }
    Putstr(com2, com2msgStart, com2msg-com2msgStart);
  }
}

int startUserInterfaceTask() {
  numUpdated = 0;
  debugUpdateNum = 0;
  return Create(11, userInterface);
}
