#include "Train.h"
#include <IoServer.h>
#include <ts7200.h>
#include <util.h>
#include <TimeServer.h>
#include <NameServer.h>

static int switchStatus[NUM_SWITCHES];
static unsigned int last_switch_write_time;

typedef struct Train {
  int speed;
  int reversed;
  int delay;
} Train;

static int solenoidTaskId;
static int com1;
static Train train[NUM_TRAINS];

void trainGetSwitch();
void trainSetSwitch();
void trainSetSpeed();

void trainController() {
  char com1Name[] = IOSERVERCOM1_NAME;
  com1 = WhoIs(com1Name);

  for (int i = 0; i <= NUM_TRAINS; i++) {
    train[i].speed = 0;
    train[i].reversed = 0;
  }

  solenoidTaskId = 0;
  last_switch_write_time = 0;

  char trainName[] = TRAIN_NAME;
  RegisterAs(TRAIN_NAME);

  for (int i = 0; i < 10;i++) {
    int tid = -1;
    char msg[8];
    //Receive(&tid, (char*)msg, 8);
    Putc(com1, 't');
  }
}


#if 0
void turnoffSolenoid(void* unused) {
  solenoidTaskId = 0;
  bwputc(COM1, SOLENOID_OFF);
}

int train_getswitch(int sw) {
  return switchStatus[sw];
}

static void setswitch(void* raw_swstate) {
  unsigned int swstate = (unsigned int) raw_swstate;
  int sw = swstate >> 16;
  int state = swstate & 0xff;

  if (state == STRAIGHT) {
    Putc(COM1, SWITCH_STRAIGHT);
  } else if (state == CURVED) {
    bwputc(COM1, SWITCH_CURVED);
  }

  bwputc(COM1, sw);
  timerRemoveTask(solenoidTaskId);
  solenoidTaskId = timerCreateTask(turnoffSolenoid, NULL, SWITCH_DELAY + 100);
  switchStatus[sw] = state;
}

void train_setswitch(int sw, int state) {
  int curtime = timerGetTime();
  int trigger_time = MAX(last_switch_write_time + SWITCH_DELAY, timerGetTime());
  last_switch_write_time = trigger_time;
  int wait_time = MAX(trigger_time - curtime, 0);

  timerCreateTask(setswitch, (void*)((sw << 16) | (unsigned int) state), wait_time);
}

void train_setspeed(int train, int spd) {
  bwputc(COM1, spd);
  bwputc(COM1, train);
  speed[train] = spd;

  timerRemoveTask(delay[train]);
  delay[train] = 0;
}

static void train_speed(void* p) {
  unsigned int data = (unsigned int)p;
  int reverse = 0x80000000 & data;
  int train = 0x7fff & (data >> 16);
  int speed = data & 0xffff;

  if (reverse) {
    bwputc(COM1, 0xf);
    bwputc(COM1, train);
  }

  bwputc(COM1, speed);
  bwputc(COM1, train);
  delay[train] = 0;
}

void train_reverse(int train) {
  timerRemoveTask(delay[train]);
  delay[train] = 0;

  bwputc(COM1, 0);
  bwputc(COM1, train);

  // Reverse train and give some time for train to stop.
  delay[train] = timerCreateTask(
      train_speed, (void*)(0x80000000 | (train<<16) | speed[train]), 3000);
}
#endif

int startTrainController() {
  return Create(2, trainController);
}


