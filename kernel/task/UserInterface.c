#include "UserInterface.h"
#include <util.h>
#include <NameServer.h>
#include <Train.h>
#include <TimeServer.h>
#include <IoServer.h>
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

static char* updateSensor(int box, int val, char* msg) {
	const int startX = UI_WIDTH / 2 + 1;

  // SaveCursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 's';

  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = '1'; // line
  *msg++ = ';';
  *msg++ = '0' + startX/10; // Col
  *msg++ = '0' + startX%10;
  *msg++ = 'f';

  // Print sensor info
	*msg++ = '0' + box;
	*msg++ = ':';
  if (val > 10) {
	  *msg++ = '0'+ val;
  }
  *msg++ = '0'+ val%10;

  // Restore Cursor
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = 'u';
  return msg;
}

#if 0
static void ui_switch_draw()
{
	moveCursor(1,1);

	bwputstr(COM2, "Switch");

	for (int i = 1; i<= 18; i++) {
		moveCursor(i + 1,1);
		bwputstr(COM2, "Sw   ");
		pad2(i);
		bwputstr(COM2, ": ");
		bwputc(COM2, train_getswitch(i) == STRAIGHT ? '-' : '~');
	}

	for (int i = 0; i < 4; i++) {
		moveCursor(2 + i, 21);
		bwputstr(COM2, "Sw 0x");
    if (i == 0) {
      bwputc(COM2, '9');
    } else if (i == 1) {
      bwputc(COM2, 'a');
    } else if (i == 2) {
      bwputc(COM2, 'b');
    } else if (i == 3) {
      bwputc(COM2, 'c');
    }
		bwputc(COM2, ':');
		bwputc(COM2, ' ');
		bwputc(COM2, train_getswitch(i) == STRAIGHT ? '-' : '~');
	}
}

static void ui_switch_redraw(int sw, int ss) {
	char msg = ss == STRAIGHT ? '-' : '~';

	if (sw >= 1 && sw <= 18) {
		moveCursor(sw + 1,10);
	}
	else if (sw >= 0x9a && sw <= 0x9d) {
		moveCursor(sw - 0x99 + 1, 30);
	}
  bwputc(COM2, msg);
}

#endif

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
  char msgStart[255];
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

  displayStaticContent(com2);

  char com2msgStart[128];
  for (;;) {
    int tid = -1;
    UiMsg msg;
    char* com2msg = com2msgStart;
    Receive(&tid, (char*)&msg, sizeof(UiMsg));
    Reply(tid, (char*)1, 0);

    switch (msg.type) {
      case PROMPT_CHAR: {
        com2msg = promptChar(msg.data2+1, msg.data3, com2msg);
        break;
      }
      case UPDATE_TIME: {
        com2msg = updateTime(msg.data3, com2msg);
        break;
      }
      case UPDATE_SENSOR: {
        // TODO, why is val wrong?
        com2msg = updateSensor(msg.data1 /*box*/, msg.data2 /*val*/, com2msg);
        break;
      }
    }
    Putstr(com2, com2msgStart, com2msg-com2msgStart);
  }
}

int startUserInterfaceTask() {
  return Create(11, userInterface);
}
