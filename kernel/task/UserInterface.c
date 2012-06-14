#include "UserInterface.h"
#include <util.h>
#include <NameServer.h>
#include <Train.h>
#include <TimeServer.h>
#include <IoServer.h>

#define REFRESH_TICK 100
#define NUM_SENSORSET 5
#define NUM_SENSOR_STATUS 21
#define UI_WIDTH 80
#define PROMPT_R1 '0'+2
#define PROMPT_R2 '0'+7
#define CLOCK_R1 '0'+2
#define CLOCK_R2 '0'+6
#define CLOCK_C1 '0'
#define CLOCK_C2 '0'+1

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

static char* updateTime(UiMsg* ui, char* msg) {
  unsigned int ms = ui->data3;
  int min = (ms / 60000) % 60;
  int sec = (ms / 1000) % 60;
  int mso = (ms / 100) % 10;

  // move to position
  *msg++ = ESC;
  *msg++ = '[';
  *msg++ = CLOCK_R1;
  *msg++ = CLOCK_R2;
  *msg++ = ';';
  *msg++ = CLOCK_C1;
  *msg++ = CLOCK_C2;
  *msg++ = 'f';

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

#if 0
static void ui_sensor_draw() {
	int startX = UI_WIDTH / 2 + 1;

	moveCursor(1, startX);

	bwputstr(COM2, "Sensor");

	for (int i = startX + 8; i < UI_WIDTH - 17; i++) {
		bwputc(COM2, ' ');
	}
}

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

void uiInit() {
  // Hide cursor
  bwputstr(COM2, "\033[?25");
  bwputc(COM2, 'l');
  // Clear console
  bwputstr(COM2, "\033c");
  bwputstr(COM2, "\033[2J");

  ui_switch_draw();
	ui_sensor_draw();
	moveCursor(29, 1);
	bwputc(COM2, '>');
}

static int execute(char* cmd) {
  bwputc(COM2, '\033');
  bwputc(COM2, '[');
  bwputc(COM2, '2');
  bwputc(COM2, '8');
  bwputc(COM2, ';');
  bwputc(COM2, '1');
  bwputc(COM2, 'f');
	bwputstr(COM2, "\033[K");
  bwputc(COM2, '\033');
  bwputc(COM2, '[');
  bwputc(COM2, '2');
  bwputc(COM2, '8');
  bwputc(COM2, ';');
  bwputc(COM2, '1');
  bwputc(COM2, 'f');

	if (cmd[0] == 'q' && cmd[1] == '\0') {
    return QUIT;
  } else if (cmd[0] == 't' && cmd[1] == 'r' && cmd[2] == ' ') {
    cmd += 3;

    int train_number = parseInt(cmd);

    if (train_number >= 1 && train_number <= 80) {
      cmd += 1 + findFirstNonDigit(cmd);

      int train_speed = parseInt(cmd);

      if (train_speed >= 0 && train_speed <= 0xe) {
        train_setspeed(train_number, train_speed);
        bwprintf(COM2, "Set train %d's speed to %d.", train_number, train_speed);
        return 0;
      }
    }
  } else if (cmd [0] == 'r' && cmd[1] == 'v' && cmd[2] == ' ') {
    cmd += 3;

    int train_number = parseInt(cmd);

    if (train_number >= 1 && train_number <= 80) {
      train_reverse(train_number);
      bwprintf(COM2, "Reversed train %d's direction.", train_number);
      return 0;
    }
  } else if (cmd[0] == 's' && cmd[1] == 'w') {
    cmd += 3;
    int switch_number = parseInt(cmd);

    if (switch_number >= 0 && switch_number <= 0xff) {
      cmd += 1 + findFirstNonDigit(cmd);

      int ss = -1;

      if (*cmd == 's' || *cmd == 'S') {
        ss = STRAIGHT;
      } else if (*cmd == 'c' || *cmd == 'C') {
        ss = CURVED;
      }
      if (ss != -1) {
        train_setswitch(switch_number, ss);
        ui_switch_redraw(switch_number, ss);
        return 0;
      }
    }
  }
	bwprintf(COM2, "Invalid comand.");
  return INVALID_CMD;
}
#endif

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

void userInterface() {
  char name[] = UI_TASK_NAME;
  RegisterAs(name);
  char com2name[] = IOSERVERCOM2_NAME;
  int com2 = WhoIs(com2name);

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
      }
    }
    Putstr(com2, com2msgStart, com2msg-com2msgStart);
  }
}

int startUserInterfaceTask() {
  return Create(11, userInterface);
}
