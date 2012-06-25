#include <track_data.h>
#include <track_node.h>
#include <Track.h>
#include <NameServer.h>
#include <IoServer.h>
#include <IoHelper.h>
#include <UserInterface.h>
#include <ts7200.h>
#include <util.h>

extern int CALIBRATION;
static int switchStatus[NUM_SWITCHES];
static int com1;

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
  if (depth > 7) {
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

static int findNextSensor(track_node* from, TrackLandmark* dst) {
  track_node* currentNode = from;
  int dist = 0;
  while(currentNode->type != NODE_EXIT) {
    track_edge edge;
    if (currentNode->type == NODE_BRANCH) {
      int switch_num = currentNode->num;
      if (switchStatus[switch_num] ==  SWITCH_CURVED) {
        edge = currentNode->edge[DIR_CURVED];
      } else {
        edge = currentNode->edge[DIR_STRAIGHT];
      }
    } else {
      edge = currentNode->edge[DIR_AHEAD];
    }
    dist += edge.dist;
    currentNode = edge.dest;

    if (currentNode->type == NODE_SENSOR) {
      dst->type = LANDMARK_SENSOR;
      dst->num1 = currentNode->num / 16;
      dst->num2 = currentNode->num % 16 + 1;
      return dist;
    }
  }
  return -1;
}

static void trackSetSwitch(int sw, int state) {
  char msg[2];
  msg[0] = (char)state;
  msg[1] = (char)sw;

  Putstr(com1, msg, 2);
  switchStatus[sw] = state;
}

static void trackController() {
  char trackName[] = TRACK_NAME;
  RegisterAs(trackName);

  char com1Name[] = IOSERVERCOM1_NAME;
  com1 = WhoIs(com1Name);

  char uiName[] = UI_TASK_NAME;
  int ui = -1;
  if (!CALIBRATION) {
    ui = WhoIs(uiName);
  }

  if (!CALIBRATION) {
    for (int i = 1; i < 19; i++) {
      trackSetSwitch(i, SWITCH_CURVED);
    }
    for (int i = 153; i< 157; i++) {
      trackSetSwitch(i, SWITCH_CURVED);
    }
  }

  track_node track[TRACK_MAX];
  init_trackb(track);
  UiMsg uimsg;
  for ( ;; ) {
    int tid = -1;
    TrackMsg msg;
    Receive(&tid, (char*)&msg, sizeof(TrackMsg));
    switch (msg.type) {
      case GET_SWITCH: {
        Reply(tid, (char*)(switchStatus + msg.data), 4);
        break;
      }
      case SET_SWITCH: {
        Reply(tid, (char*)1, 0);
        TrackLandmark sw = msg.landmark1;
        trackSetSwitch((int)sw.num2, (int)msg.data);
        if (!CALIBRATION) {
          uimsg.type = UPDATE_SWITCH;
          uimsg.data1 = sw.num2;
          uimsg.data2 = msg.data;
          Send(ui, (char*)&uimsg, sizeof(UiMsg), (char*)1, 0);
        }
        break;
      }
      case QUERY_DISTANCE: {
        track_node* from = findNode(track, msg.landmark1);
        track_node* to = findNode(track, msg.landmark2);
        int distance = calculateDistance(from, to, 0);
        Reply(tid, (char *)&distance, sizeof(int));
        break;
      }
      case QUERY_NEXT_SENSOR: {
        track_node* from = findNode(track, msg.landmark1);
        TrackLandmark nextSensor = {0, 0, 0};
        int distance = findNextSensor(from, &nextSensor);

        TrackNextSensorMsg sensorMsg;
        sensorMsg.sensor = nextSensor;
        sensorMsg.dist = distance;

        Reply(tid, (char *)&sensorMsg, sizeof(TrackNextSensorMsg));
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
