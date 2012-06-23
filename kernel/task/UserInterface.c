#include "UserInterface.h"
#include <util.h>
#include <NameServer.h>
#include <Train.h>
#include <TimeServer.h>
#include <IoServer.h>
#include <IoHelper.h>
#include <Sensor.h>

#define REFRESH_TICK 100
#define NUM_SENSORSET 5
#define NUM_SENSOR_STATUS 21
#define UI_WIDTH 80
#define PROMPT_R1 '0'+2
#define PROMPT_R2 '0'+7
#define CLOCK_R1 '0'+2
#define CLOCK_R2 '0'+6
#define CLOCK_C1 '1'

static char* promptChar(char col, char c, char* msg) {
  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = PROMPT_R1;
  *msg++ = PROMPT_R2;
  *msg++ = ';';
  if (col / 100 != 0) {
    *msg++ = '1'; // Assume dont have greater than 100.
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
    // Clear line
    *msg++ = ESC;
    *msg++ = '[';
    *msg++ = 'K';
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
  int updateRow = debugUpdateNum%8 + 29;

  // SaveCursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 's';

  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = '0' + updateRow/10; // line
  *msg++ = '0'+ updateRow%10; // line
  *msg++ = ';';
  *msg++ = '1'; // Col
  *msg++ = 'f';

  for (int i = 0; i < len; i++) {
    *msg++ = receive[i];
  }

  // Restore Cursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 'u';

  debugUpdateNum += 1;
  return msg;
}

static char* updateTime(unsigned int ms, char* msg) {
  int min = (ms / 60000) % 60;
  int sec = (ms / 1000) % 60;
  int mso = (ms / 100) % 10;
  // SaveCursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 's';

  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = CLOCK_R1;
  *msg++ = CLOCK_R2;
  *msg++ = ';';
  *msg++ = CLOCK_C1;
  *msg++ = 'f';

  // Print the time
	*msg++ = min/10 + '0';
	*msg++ = min%10 + '0';
	*msg++ = ':';
	*msg++ = sec/10 + '0';
	*msg++ = sec%10 + '0';
	*msg++ = '.';
	*msg++ = mso + '0';

  // Restore Cursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 'u';

  return msg;
}

static char* updateIdle(int percentage, char* msg) {
  // SaveCursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 's';

  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = CLOCK_R1;
  *msg++ = CLOCK_R2-1;
  *msg++ = ';';
  *msg++ = CLOCK_C1;
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

  // Restore Cursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 'u';

  return msg;
}


static int numUpdated;
static char* updateSensor(int box, int val, char* msg) {
  int updateRow = (numUpdated % 16)+2;

  // SaveCursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 's';

  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  if (updateRow >= 10) {
    *msg++ = '1'; // line
  }
  *msg++ = '0' + updateRow%10; // line
  *msg++ = ';';
  *msg++ = '4';
  *msg++ = '1';
  *msg++ = 'f';

  // Print sensor info
	*msg++ = 'A' + box;
	*msg++ = ':';
  if (val >= 10) {
	  *msg++ = '0'+ val/10;
  }
  *msg++ = '0'+ val%10;
  *msg++ = ' '; // Extra space to clear previous char on screen.

  // Restore Cursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 'u';

  numUpdated++;
  return msg;
}

static char* drawSwitches(char* msg) {
  // Move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = '1'; // line
  *msg++ = ';';
  *msg++ = '1'; // Col
  *msg++ = 'f';

  // Text
  *msg++ = 'S';
  *msg++ = 'w';
  *msg++ = 'i';
  *msg++ = 't';
  *msg++ = 'c';
  *msg++ = 'h';

	for (int i = 1; i <= 18; i++) {
    // Move again
    *msg++ = ESC;
    *msg++ = '[';
    msg = pad2(i+1, msg); // Row
    *msg++ = ';';
    *msg++ = '1'; // Col
    *msg++ = 'f';

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
    // Move cursor
    *msg++ = ESC;
    *msg++ = '[';
    *msg++ = '2';
    *msg++ = '0'+ i%10;
    *msg++ = ';';
    *msg++ = '1'; // Col
    *msg++ = 'f';

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
  // SaveCursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 's';

	char state = ss == SWITCH_STRAIGHT ? '-' : '~';

  if (sw >= 0x99 && sw <= 0x9c) {
    sw -= 134; // make 0x99 19
  }
  sw ++;

  *msg++ = ESC;
  *msg++ = '[';
  if (sw >= 10) {
    *msg++ = '0' + sw/10;
  }
  *msg++ = '0' + sw%10;
  *msg++ = ';';
  *msg++ = '8'; // Col
  *msg++ = 'f';

  *msg++ = state;

  // Restore Cursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 'u';

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

  for (;;) {
    SensorMsg sensorMsg;
    sensorMsg.type = QUERY_RECENT;
    Sensor sensor;
    Send(sensorServer, (char*)&sensorMsg, sizeof(SensorMsg),
        (char*)&sensor, sizeof(Sensor));

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
	const int startX = UI_WIDTH / 2 + 1;
  // Move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = '1'; // line
  *msg++ = ';';
  *msg++ = '0'+startX/10; // Col
  *msg++ = '0'+startX%10; // Col
  *msg++ = 'f';

  // Text
  *msg++ = 'S';
  *msg++ = 'e';
  *msg++ = 'n';
  *msg++ = 's';
  *msg++ = 'o';
  *msg++ = 'r';

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
        //printff(com2, "\nUpdate sensor %d %d\n", msg->data1, msg->data2);
        com2msg = updateSensor(msg->data1 /*box*/, msg->data2 /*val*/, com2msg);
        break;
      }
      case UPDATE_SWITCH: {
        //printff(com2, "\nUpdate switch %d %d\n", msg->data1, msg->data2);
        com2msg = updateSwitch(msg->data1 /*switch*/, msg->data2 /*state*/, com2msg);
        break;
      }
      case UPDATE_IDLE: {
        com2msg = updateIdle(Idleness(), com2msg);
      }
      default: {
        com2msg = updateDebugMessage(receiveBuffer, com2msg, len);
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
