#ifndef TIMESERVER_H_
#define TIMESERVER_H_

int createTimeserver();

// Returns.
// 0 – success.
//-1 – if the clock server task id inside the wrapper is invalid.
//-2 – if the clock server task id inside the wrapper is not the id of the clock server
int Delay(int ticks);

// Returns:
// number of 10ms ticks since the clock server was started.
int Time();

// Returns
int DelayUntil(int ticks);

void timernotifier_task();

void timeserver_task();


#endif // TIMESERVER_H_
