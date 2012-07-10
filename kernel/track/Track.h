#ifndef TRACK_H_
#define TRACK_H_

#define TRACK_NAME "TRACKK\0"
#define REVERSE -1

#define NUM_SWITCHES 0xff
#define SWITCH_STRAIGHT 35
#define SWITCH_CURVED   34

#define UNRESERVED -1
#define RESERVE_SUCESS 0
#define RESERVE_FAIL   1

#define GET_SWITCH 0
#define SET_SWITCH 1
//#define QUERY_DISTANCE 2
#define QUERY_NEXT_SENSOR_FROM_SENSOR 3
#define QUERY_NEXT_SENSOR_FROM_POS 4
#define ROUTE_PLANNING 5
#define SET_TRACK 6
#define RELEASE_OLD_N_RESERVE_NEW 7

typedef enum {
  LANDMARK_SENSOR,
  LANDMARK_SWITCH,
  LANDMARK_END,
  LANDMARK_FAKE,
} LandmarkType;

#define MR 0
#define BR 1

#define EN 0
#define EX 1

typedef struct TrackLandmark {
  LandmarkType type;
  char num1; //MR vs BR, EN vs EX, 0 vs 1 vs 2 vs 3 vs 4
  char num2; //a number
} TrackLandmark;

typedef struct ReleaseOldAndReserveNewTrackMsg {
  char type;
  char trainNum;
  int stoppingDistance;
  char preSensorLen;
  TrackLandmark lastSensor;
  TrackLandmark predSensor[10];
} ReleaseOldAndReserveNewTrackMsg;

typedef struct Position {
  TrackLandmark landmark1;
  TrackLandmark landmark2;
  int offset;
} Position; // position on track is represented by two consecutive sensors on a offset from the first

typedef struct TrackMsg {
  char type;
  char data;
  TrackLandmark landmark1;
  Position position1;
  Position position2;
} TrackMsg;

typedef struct TrackSensorPrediction {
  TrackLandmark sensor;   //
  int dist;               // In mm
  TrackLandmark conditionLandmark; // either switch or sensor
  int condition; // SWITCH_CURVED/SWITCH_STRAIGHT if condition is switch, -1 if condition sensor, primary doesn't have condition
} TrackSensorPrediction;

typedef struct TrackNextSensorMsg {
  int numPred;
  TrackSensorPrediction predictions[20]; // [0] is primary, all other is secondary
} TrackNextSensorMsg;

typedef struct RouteNode {
  TrackLandmark landmark;
  int num; // num = -1 if reverse command, num = SWITCH_CURVED/SWITCH_STRAIGHT if landmark is BR switch,
  int dist;
} RouteNode;

#define MAX_ROUTE_NODE 150
#define SAFE_REVERSE_DIST 320 // TODO(cao) too generous?

typedef struct Route {
  int dist;
  int length;
  RouteNode nodes[150];
} Route;

int startTrackManagerTask();

#endif
