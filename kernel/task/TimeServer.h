#ifndef TIMESERVER_H_
#define TIMESERVER_H_

#define TIMESERVER_NAME "TS\0"
#define T100MS 10

int startTimeServerTask();

// Returns.
// 0 – success.
//-1 – if the clock server task id inside the wrapper is invalid.
//-2 – if the clock server task id inside the wrapper is not the id of the clock server
int Delay(int ticks, int timeServerTid);

// number of 10ms ticks since the clock server was started.
int Time(int timeServerTid);

int DelayUntil(int ticks, int timeServerTid);

#endif // TIMESERVER_H_
