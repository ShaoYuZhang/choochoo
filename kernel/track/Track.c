#include <track_data.h>
#include <track_node.h>
#include <Track.h>
#include <NameServer.h>
#include <IoHelper.h>
#include <ts7200.h>
#include <util.h>

static int get_node_type(TrackLandmark landmark) {
  int type;
  switch(landmark.type) {
    case LANDMARK_SENSOR: {
      type = NODE_SENSOR;
      break;
    }
    case LANDMARK_SWITCH: {
      if (landmark.num1 == MR) {
        type = NODE_MERGE;
      }
      else {
        type = NODE_BRANCH;
      }
      break;
    }
    case LANDMARK_END: {
      if (landmark.num1 == EN) {
        type = NODE_ENTER;
      } else {
        type = NODE_EXIT;
      }
      break;
    }
    default: {
      ASSERT(FALSE, "Not suppported Landmark type.");
    }
  }
  return type;
}

static int get_node_num(TrackLandmark landmark) {
  int num;
  switch(landmark.type) {
    case LANDMARK_SENSOR: {
      num = (landmark.num1) * 16 + landmark.num2 - 1;
      break;
    }
    case LANDMARK_SWITCH: {
      num = landmark.num2;
      break;
    }
    case LANDMARK_END: {
      num = landmark.num2;
      break;
    }
    default: {
      ASSERT(FALSE, "Not suppported Landmark type.");
    }
  }
  return num;
}

// TODO slow, should worry about it?
static track_node* findNode(track_node* track, TrackLandmark landmark) {
  int node_type = get_node_type(landmark);
  int node_num = get_node_num(landmark);
  for (int i = 0 ; i < TRACK_MAX; i++) {
    if (track[i].type == NODE_NONE) {
      continue;
    }
    if (track[i].type == node_type && track[i].num == node_num) {
      return &track[i];
    }
  }
  return (track_node*) NULL;
}

// TODO currently only support landmarks that are relatively close to each other, arbitrary landmark require route planning
static int calculateDistance(track_node* currentNode, track_node* targetNode, int depth) {
  if (depth > 5) {
    return -1;
  }
  if (currentNode == targetNode) {
    return 0;
  }
  if (currentNode->type == NODE_EXIT) {
    return -1;
  } else if (currentNode->type != NODE_BRANCH) {
    track_edge edge = currentNode->edge[0];
    int dist = calculateDistance(edge.dest, targetNode, depth + 1);
    if (dist == -1) {
      return -1;
    } else {
      return edge.dist + dist;
    }
  } else {
    track_edge edge1 = currentNode->edge[0];
    track_edge edge2 = currentNode->edge[1];
    int dist1 = calculateDistance(edge1.dest, targetNode, depth + 1);
    int dist2 = calculateDistance(edge2.dest, targetNode, depth + 1);
    if (dist1 != -1) {
      return edge1.dist + dist1;
    } else if (dist2 != -1) {
      return edge2.dist + dist2;
    } else {
      return -1;
    }
  }
}

static void trackController() {
  char trackName[] = TRACK_NAME;
  RegisterAs(trackName);
  track_node track[TRACK_MAX];
  init_tracka(track);
  for ( ;; ) {
    int tid = -1;
    TrackMsg msg;
    Receive(&tid, (char*)&msg, sizeof(TrackMsg));
    switch (msg.type) {
      case QUERY_DISTANCE: {
        track_node* from = findNode(track, msg.landmark1);
        track_node* to = findNode(track, msg.landmark2);
        int distance = calculateDistance(from, to, 0);
        Reply(tid, (char *)&distance, sizeof(int));
        break;
      }
      default: {
        ASSERT(FALSE, "Not suppported track message type.");
      }
    }
  }
}

int startTrackManagerTask() {
  return Create(4, trackController);
}
