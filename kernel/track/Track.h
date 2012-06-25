#ifndef TRACK_H_
#define TRACK_H

#define TRACK_NAME "TRACKK\0"

#define NUM_SWITCHES 0xff
#define SWITCH_STRAIGHT 35
#define SWITCH_CURVED   34

#define GET_SWITCH 0
#define SET_SWITCH 1
#define QUERY_DISTANCE 2
#define QUERY_NEXT_SENSOR 3

typedef enum {
  LANDMARK_SENSOR,
  LANDMARK_SWITCH,
  LANDMARK_END,
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

typedef struct TrackMsg {
  int type;
  TrackLandmark landmark1;
  TrackLandmark landmark2;
  char data;
} TrackMsg;

typedef struct TrackNextSensorMsg {
  TrackLandmark sensor;
  int dist;
} TrackNextSensorMsg;

int startTrackManagerTask();

#endif
