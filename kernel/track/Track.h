#ifndef TRACK_H_
#define TRACK_H_

#define TRACK_NAME "TRACKK\0"
#define REVERSE -1

#define NUM_SWITCHES 0xff
#define SWITCH_STRAIGHT 35
#define SWITCH_CURVED   34

#define GET_SWITCH 0
#define SET_SWITCH 1
#define QUERY_DISTANCE 2
#define QUERY_NEXT_SENSOR 3
#define ROUTE_PLANNING 4

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

typedef struct Position {
  TrackLandmark landmark1;
  TrackLandmark landmark2;
  int offset;
} Position; // position on track is represented by two consecutive sensors on a offset from the first

typedef struct TrackMsg {
  int type;
  TrackLandmark landmark1;
  Position position1;
  Position position2;
  char data;
} TrackMsg;

typedef struct TrackNextSensorMsg {
  TrackLandmark sensor;   //
  int dist;               // In mm
} TrackNextSensorMsg;

typedef struct RouteNode {
  TrackLandmark landmark;
  int num; // num = -1 if reverse command, num = SWITCH_CURVED/SWITCH_STRAIGHT if landmark is BR switch,
  int dist;
} RouteNode;

#define MAX_ROUTE_NODE 150
#define SAFE_REVERSE_DIST 160 + 60 // TODO(cao) too generous?

typedef struct Route {
  int dist;
  int length;
  RouteNode nodes[150];
} Route;

int startTrackManagerTask();

#endif
