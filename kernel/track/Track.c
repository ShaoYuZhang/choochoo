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

static TrackLandmark getLandmark(track_node* node) {
  TrackLandmark landmark;
  if (node->type == NODE_SENSOR) {
    landmark.type = LANDMARK_SENSOR;
    landmark.num1 = node->num / 16;
    landmark.num2 = node->num % 16 + 1;
  } else if (node->type == NODE_BRANCH) {
    landmark.type = LANDMARK_SWITCH;
    landmark.num1 = BR;
    landmark.num2 = node->num;
  } else if (node->type == NODE_MERGE) {
    landmark.type = LANDMARK_SWITCH;
    landmark.num1 = MR;
    landmark.num2 = node->num;
  } else if (node->type == NODE_ENTER) {
    landmark.type = LANDMARK_END;
    landmark.num1 = EN;
    landmark.num2 = node->num;
  } else if (node->type == NODE_EXIT) {
    landmark.type = LANDMARK_END;
    landmark.num1 = EX;
    landmark.num2 = node->num;
  } else if (node->type == NODE_FAKE) {
    landmark.type= LANDMARK_FAKE;
    landmark.num1 = 0;
    landmark.num2 = 0;
  } else {
    ASSERT(FALSE, "invalid track node");
  }
  return landmark;
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

// only support landmarks that are relatively close
static int traverse(track_node* currentNode, track_node* targetNode, int depth, int max_depth, track_edge** edges) {
  if (currentNode == targetNode) {
    return depth;
  }
  if (depth == max_depth) {
    return -1;
  }

  if (currentNode->type == NODE_EXIT) {
    return -1;
  } else if (currentNode->type != NODE_BRANCH) {
    track_edge* edge = &currentNode->edge[DIR_AHEAD];
    int len = traverse(edge->dest, targetNode, depth + 1, max_depth, edges+1);
    if (len != -1) {
      *edges = edge;
      return len;
    } else {
      return -1;
    }
  } else {
    track_edge* edge1 = &currentNode->edge[DIR_STRAIGHT];
    track_edge* edge2 = &currentNode->edge[DIR_CURVED];
    int len1 = traverse(edge1->dest, targetNode, depth + 1, max_depth, edges+1);
    int len2 = traverse(edge2->dest, targetNode, depth + 1, max_depth, edges+1);
    if (len1 != -1) {
      *edges = edge1;
      return len1;
    } else if (len2 != -1) {
      *edges = edge1;
      return len2;
    } else {
      return -1;
    }
  }
}

static int locateNode(track_node* track, Position pos, track_edge** edge) {
  track_node* sensor1 = findNode(track, pos.landmark1);
  track_node* sensor2 = findNode(track, pos.landmark2);

  track_edge* edges[7];
  int len = traverse(sensor1, sensor2, 0, 7, edges);
  if (len > 0) {
    int dist = pos.offset;
    for (int i = 0; i < len; i++) {
      if (dist < edges[i]->dist) {
        *edge = edges[i];
        return dist;
      }
      else {
        dist -= edges[i]->dist;
      }
    }
  }
  return -1;
}

static int calculateDistance(track_node* currentNode, track_node* targetNode) {
  track_edge* edges[7];
  int len = traverse(currentNode, targetNode, 0, 7, edges);
  if (len == -1) {
    return -1;
  } else {
    int dist = 0;
    for (int i = 0; i < len; i++) {
      dist += edges[i]->dist;
    }
    return dist;
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
      *dst = getLandmark(currentNode);
      return dist;
    }
  }
  return -1;
}

static int edgeType(track_edge* edge) {
  if (edge->src->type != NODE_BRANCH) {
    return DIR_AHEAD;
  } else if (&edge->src->edge[DIR_STRAIGHT] == edge) {
    return DIR_STRAIGHT;
  } else {
    return DIR_CURVED;
  }
}

static void fakeNode(track_edge* edge, track_node* fake1, track_node* fake2, int offset) {
  int dirType;

  dirType = edgeType(edge->reverse);
  fake1->name = "";
  fake1->type = NODE_FAKE;
  fake1->reverse = fake2;
  fake1->edge[DIR_AHEAD].reverse = &edge->dest->reverse->edge[dirType];
  fake1->edge[DIR_AHEAD].src = fake1;
  fake1->edge[DIR_AHEAD].dest = edge->dest;
  fake1->edge[DIR_AHEAD].dist = edge->dist - offset;

  edge->dest->reverse->edge[dirType].reverse = &fake1->edge[DIR_AHEAD];
  edge->dest->reverse->edge[dirType].dest = fake2;
  edge->dest->reverse->edge[dirType].dist = edge->dist -offset;

  dirType = edgeType(edge);
  fake2->name = "";
  fake2->type = NODE_FAKE;
  fake2->reverse = fake1;
  fake2->edge[DIR_AHEAD].reverse = &edge->src->reverse->edge[dirType];
  fake2->edge[DIR_AHEAD].src = fake2;
  fake2->edge[DIR_AHEAD].dest = edge->reverse->dest;
  fake2->edge[DIR_AHEAD].dist = offset;

  edge->src->edge[dirType].reverse = &fake2->edge[DIR_AHEAD];
  edge->src->edge[dirType].dest = fake1;
  edge->src->edge[dirType].dist = offset;
}

static void computeSafeReverseDistHelper(track_edge* edge) {
  int new_safe = edge->src->safe_reverse_dist - edge->dist;
  if (new_safe > 0) {
    edge->src->safe_reverse_dist = INT_MAX;
    edge->dest->safe_reverse_dist = MAX(new_safe, edge->dest->safe_reverse_dist);
    if (edge->dest->type == NODE_BRANCH) {
      computeSafeReverseDistHelper(&edge->dest->edge[DIR_CURVED]);
    } else if (edge->dest->type == NODE_EXIT) {
      computeSafeReverseDistHelper(&edge->dest->edge[DIR_AHEAD]);
    }
  }
}

static void computeSafeReverseDist(track_node* track) {
  for (int i = 0 ; i < TRACK_MAX + 4; i++) {
    if (track[i].type == NODE_BRANCH) {
      track[i].safe_reverse_dist = SAFE_REVERSE_DIST;
      computeSafeReverseDistHelper(&track[i].edge[DIR_STRAIGHT]);
      computeSafeReverseDistHelper(&track[i].edge[DIR_CURVED]);
    } else if (track[i].type == NODE_MERGE) {
      track[i].safe_reverse_dist = SAFE_REVERSE_DIST;
      computeSafeReverseDistHelper(&track[i].edge[DIR_AHEAD]);
    }
  }
  for (int i = 0 ; i < TRACK_MAX + 4; i++) {
    if (track[i].type != NODE_NONE) {
      track[i].safe_reverse_dist = MAX(track[i].safe_reverse_dist, (track[i].reverse)->safe_reverse_dist);
    }
  }
}

// Dijkstra's algorithm, currently slow, need a heap for efficiency
static void findRoute(track_node* track, Position from, Position to, Route* result) {
  // fake position into graph
  track_edge* fromEdge = (track_edge*)NULL;
  track_edge* toEdge = (track_edge*)NULL;
  int offsetFrom = locateNode(track, from, &fromEdge);
  int offsetTo = locateNode(track, to, &toEdge);

  if (offsetFrom == -1 || offsetTo == -1) {
    result->dist = 0;
    result->length = 0;
    return;
  }

  track_node fromNodeSrc = *fromEdge->src;
  track_node fromNodeReverseSrc = *fromEdge->reverse->src;
  track_node toNodeSrc = *toEdge->src;
  track_node toNodeReverseSrc = *toEdge->reverse->src;

  fakeNode(fromEdge, &track[TRACK_MAX], &track[TRACK_MAX + 1], offsetFrom);
  fakeNode(toEdge, &track[TRACK_MAX + 2], &track[TRACK_MAX + 3], offsetTo);

  track_node *fromNode = &track[TRACK_MAX];
  track_node *toNode = &track[TRACK_MAX + 2];
  for (int i = 0 ; i < TRACK_MAX + 4; i++) {
    if (track[i].type == NODE_NONE) {
      continue;
    }
    track[i].curr_dist = INT_MAX;
    track[i].in_queue = 1;
    track[i].route_previous = (track_node*)NULL;
    track[i].safe_reverse_dist = 0;
  }

  computeSafeReverseDist(track);

  fromNode->curr_dist = 0;

  // Dijkstra's
  while (1) {
    int min_dist = INT_MAX;
    track_node* curr_node = (track_node*)NULL;
    for (int i = 0 ; i < TRACK_MAX + 4; i++) {
      if (track[i].type == NODE_NONE) {
        continue;
      }
      if (!track[i].in_queue) {
        continue;
      }

      if (track[i].curr_dist < min_dist) {
        min_dist = track[i].curr_dist;
        curr_node = &track[i];
      }
    }

    if (curr_node == (track_node*)NULL || curr_node == toNode) {
      break;
    }

    curr_node->in_queue = 0;

    track_node* neighbours[3];
    int neighbours_dist[3];
    int neighbour_count = 0;

    // reverse
    if (curr_node->safe_reverse_dist != INT_MAX && curr_node->reverse->in_queue) {
      neighbours[neighbour_count] = curr_node->reverse;
      neighbours_dist[neighbour_count++] = 2 * curr_node->safe_reverse_dist;
    }

    if (curr_node->type == NODE_BRANCH) {
      neighbours[neighbour_count] = curr_node->edge[DIR_STRAIGHT].dest;
      neighbours_dist[neighbour_count++] = curr_node->edge[DIR_STRAIGHT].dist;
      neighbours[neighbour_count] = curr_node->edge[DIR_CURVED].dest;
      neighbours_dist[neighbour_count++] = curr_node->edge[DIR_CURVED].dist;
    }
    else if (curr_node->type != NODE_EXIT) {
      neighbours[neighbour_count] = curr_node->edge[DIR_AHEAD].dest;
      neighbours_dist[neighbour_count++] = curr_node->edge[DIR_AHEAD].dist;
    }

    for (int i = 0; i < neighbour_count; i ++) {
      int temp_dist = neighbours_dist[i] + curr_node->curr_dist;
      if (temp_dist < neighbours[i]->curr_dist) {
        neighbours[i]->curr_dist = temp_dist;
        neighbours[i]->route_previous = curr_node;
      }
    }
  }

  // construct path
  RouteNode tempRoute[150];
  int index = 150;
  result->dist = toNode->curr_dist;

  // construct backwards on temp
  track_node *next_node = (track_node*)NULL;
  track_node *curr_node = toNode;
  while (curr_node != (track_node*)NULL) {
    RouteNode tempRouteNode;

    TrackLandmark landmark = getLandmark(curr_node);
    tempRouteNode.landmark = landmark;

    if (curr_node->reverse == next_node) {
      tempRouteNode.num = REVERSE;
      tempRouteNode.dist = curr_node->safe_reverse_dist;
      tempRoute[--index] = tempRouteNode;
      tempRouteNode.num = 0;
    } else if (curr_node->type == NODE_BRANCH) {
      if (curr_node->edge[DIR_STRAIGHT].dest == next_node) {
        tempRouteNode.num = SWITCH_STRAIGHT;
        tempRouteNode.dist = curr_node->edge[DIR_STRAIGHT].dist;
      } else {
        tempRouteNode.num = SWITCH_CURVED;
        tempRouteNode.dist = curr_node->edge[DIR_CURVED].dist;
      }
    } else {
      tempRouteNode.dist = curr_node->edge[DIR_AHEAD].dist;
      tempRouteNode.num = 0;
    }

    tempRoute[--index] = tempRouteNode;

    next_node = curr_node;
    curr_node = next_node->route_previous;
  }

  // copy to output
  for (int i = index; i < 150; i++) {
    result->nodes[i-index] = tempRoute[i];
  }
  result->length = 150 - index;

  // restore graph
  *toEdge->src = toNodeSrc;
  *toEdge->reverse->src = toNodeReverseSrc;

  *fromEdge->src = fromNodeSrc;
  *fromEdge->reverse->src = fromNodeReverseSrc;
  {
  char uiName[] = UI_TASK_NAME;
  int uiServer = WhoIs(uiName);
  PrintDebug(uiServer, "WHAT??");
  char timeServerName[] = TIMESERVER_NAME;
  int timeServer= WhoIs(timeServerName);
  Delay(10,timeServer);
  }
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

  track_node track[TRACK_MAX + 4]; // four fake nodes for route finding
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
        // TODO
        //track_node* from = findNode(track, msg.landmark1);
        //track_node* to = findNode(track, msg.landmark2);
        //int distance = calculateDistance(from, to, 0);
        int distance = 0;
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
      case ROUTE_PLANNING: {
        Position from = msg.position1;
        Position to = msg.position2;
        //track_node* from = findNode(track, msg.landmark1);
        //track_node* to = findNode(track, msg.landmark2);
        Route route;
        findRoute(track, from, to , &route);

        Reply(tid, (char *)&route, 8 + sizeof(RouteNode) * route.length);
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
