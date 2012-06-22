#ifndef TRACK_H_
#define TRACK_H

#define TRACK_NAME "TRACKK\0"

#define QUERY_DISTANCE 0

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
} TrackMsg;

int startTrackManagerTask();

#endif
