#ifndef SENSOR_H_
#define SENSOR_H_

#define SENSOR_NAME "SENSOR\0"

#define NUM_SENSOR_BOX 5

#define QUERY_WORKER 0
#define QUERY_RESPONSE_WORKER 1
#define QUERY_TIMEOUT_WORKER 2
#define SENSOR_COURIER 3
#define QUERY_RECENT 4
#define QUERY_RESPONSE_TIMEOUT_WORKER 5

typedef struct SensorMsg {
  char type; // Defined above
  char data;
  int time;
} SensorMsg;

// A1 is box = 0, val = 1
typedef struct Sensor {
  char box;
  char val;
  int time;
} Sensor;

typedef struct SensorWorkUnit {
  int tid;
  Sensor sensor;
} SensorWorkUnit;

int startSensorServerTask();

#endif // SENSOR_H
