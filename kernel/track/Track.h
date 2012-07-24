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
#define SUDO_SET_SWITCH 100
#define SET_SWITCH 1
  #define SET_SWITCH_NO_RESERVATION 1
  #define SET_SWITCH_SUCCESS  2
  #define SET_SWITCH_FAIL  4
#define QUERY_DISTANCE 2
#define QUERY_NEXT_SENSOR_FROM_SENSOR 3
#define QUERY_NEXT_SENSOR_FROM_POS 4
#define ROUTE_PLANNING 5
#define SET_TRACK 6
#define RELEASE_OLD_N_RESERVE_NEW 7
#define QUERY_SENSOR_RESERVED 8
#define QUERY_EDGES_RESERVED 9
#define UPDATE_SWITCH_STATE 10
#define GET_RANDOM_POSITION 11
#define GET_PRESET_ROUTE 12
#define RELEASE_ALL_RESERVATION 13

typedef enum {
  LANDMARK_SENSOR,
  LANDMARK_SWITCH,
  LANDMARK_END,
  LANDMARK_FAKE,
  LANDMARK_BAD,
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
  TrackLandmark lastSensor;
  int numPredSensor;
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
  char trainNum;
  TrackLandmark landmark1;
  Position position1;
  Position position2;
} TrackMsg;

typedef struct TrackSensorPrediction {
  TrackLandmark sensor;   //
  int dist;               // From position passed in to Sensor in mm
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

#define MAX_ROUTE_NODE 300
#define SAFE_REVERSE_DIST 280

typedef struct Route {
  int dist;
  int length;
  RouteNode nodes[MAX_ROUTE_NODE];
} Route;

int startTrackManagerTask();

void clearReservation(int trackManagerTid, int trainNum);

#endif
