#include <track_data.h>
#include <track_node.h>
#include <Track.h>
#include <NameServer.h>
#include <IoServer.h>
#include <IoHelper.h>
#include <UserInterface.h>
#include <ts7200.h>
#include <util.h>

//#define DEBUG_RESERVATION

extern int CALIBRATION;
static int switchStatus[NUM_SWITCHES];
static int com1;
static int ui;

static int get_node_type(TrackLandmark landmark);
static int get_node_num(TrackLandmark landmark);
static TrackLandmark getLandmark(track_node* node);
static track_node* findNode(track_node* track, TrackLandmark landmakr);
static int traverse(track_node* currentNode, track_node* targetNode, int depth, int max_depth, track_edge** edges);
static int locateNode(track_node* track, Position pos, track_edge** edge);
static int edgeType(track_edge* edge);

static void clearNodeReservation(track_node* node, int trainNum) {
  if (node->type == NODE_SENSOR || node->type == NODE_ENTER ||
      node->type == NODE_MERGE) {
    if (node->edge[DIR_AHEAD].reserved_train_num == trainNum) {
      node->edge[DIR_AHEAD].reserved_train_num = UNRESERVED;
    }
  } else if (node->type == NODE_BRANCH) {
    if (node->edge[DIR_CURVED].reserved_train_num == trainNum) {
      node->edge[DIR_CURVED].reserved_train_num = UNRESERVED;
    }
    if (node->edge[DIR_STRAIGHT].reserved_train_num == trainNum) {
      node->edge[DIR_STRAIGHT].reserved_train_num = UNRESERVED;
    }
  }
}

static int canReserve(track_edge* edge, int trainNum){
  return edge->reserved_train_num == UNRESERVED ||
    edge->reserved_train_num == trainNum;
}

static int reserveEdges(
    track_node* node, int trainNum, int stoppingDistance, int dryrun) {
  int status = RESERVE_SUCESS;
  if (node->type == NODE_SENSOR || node->type == NODE_BRANCH ||
      node->type == NODE_MERGE) {
    if (node->edge[DIR_AHEAD].reserved_train_num == trainNum &&
        stoppingDistance < 0) {
      return RESERVE_SUCESS;
    }
  }
  if (node->type == NODE_EXIT) return RESERVE_SUCESS;

  if (node->type == NODE_SENSOR || node->type == NODE_ENTER) {
    // Reserve the next edge.
    track_edge* e1 = &(node->edge[DIR_AHEAD]);
    track_edge* e2 = e1->reverse;

    if (canReserve(e1, trainNum) && canReserve(e2, trainNum)) {
      e1->reserved_train_num = trainNum;
      e2->reserved_train_num = trainNum;
      stoppingDistance -= e1->dist;
#ifdef DEBUG_RESERVATION
      PrintDebug(ui, "Reserve sensor edge %s by: %d %d di:%d %d", node->name,
          e1->reserved_train_num, e2->reserved_train_num, stoppingDistance, e1->dist);
#endif
      if (stoppingDistance > 0 || e1->dest->type == NODE_BRANCH || e1->dest->type == NODE_MERGE) {
        status = reserveEdges(e1->dest, trainNum, stoppingDistance, dryrun);
      }
      if (status == RESERVE_FAIL || dryrun) {
        e1->reserved_train_num = UNRESERVED;
        e2->reserved_train_num = UNRESERVED;
      }
    } else {
#ifdef DEBUG_RESERVATION
      PrintDebug(ui, "%d Reserved sensor edge %s",
          e1->reserved_train_num, node->name);
#endif
      return RESERVE_FAIL;
    }
  } else if (node->type == NODE_BRANCH) {
    // Reserve left and right
    track_edge* e1 = &(node->edge[DIR_STRAIGHT]);
    track_edge* e2 = &(node->edge[DIR_CURVED]);
    track_edge* e3 = e1->reverse;
    track_edge* e4 = e2->reverse;

    if (canReserve(e1, trainNum) && canReserve(e2, trainNum) &&
        canReserve(e3, trainNum) && canReserve(e4, trainNum)) {
#ifdef DEBUG_RESERVATION
      PrintDebug(ui, "Reserve branch switch edge %s", node->name);
#endif
      e1->reserved_train_num = trainNum;
      e2->reserved_train_num = trainNum;
      e3->reserved_train_num = trainNum;
      e4->reserved_train_num = trainNum;

      // Destination is already reserved.. recurse if dest is switch
      int tmpStopping = stoppingDistance - e1->dist;
      if (e1->dest->type == NODE_MERGE || e1->dest->type == NODE_BRANCH ||
          tmpStopping > 0) {
        status = reserveEdges(e1->dest, trainNum, tmpStopping, dryrun);
      }
      tmpStopping = stoppingDistance - e2->dist;
      if (status != RESERVE_FAIL &&
          (e2->dest->type == NODE_MERGE || e2->dest->type == NODE_BRANCH ||
          tmpStopping > 0)) {
        status = reserveEdges(e2->dest, trainNum, tmpStopping, dryrun);
      }
      if (status == RESERVE_FAIL || dryrun) {
        e1->reserved_train_num = UNRESERVED;
        e2->reserved_train_num = UNRESERVED;
        e3->reserved_train_num = UNRESERVED;
        e4->reserved_train_num = UNRESERVED;
      }
    } else {
#ifdef DEBUG_RESERVATION
      PrintDebug(ui, "%d Reserved switch edge Fail %s",
          e1->reserved_train_num, node->name);
#endif
      return RESERVE_FAIL;
    }
  } else if (node->type == NODE_MERGE) {
    track_edge* e1 = &(node->edge[DIR_AHEAD]);
    track_edge* e2 = e1->reverse;

    // Reserve BR of MR edge (so no one crashes in).
    track_edge* e3 = &(e2->dest->edge[DIR_STRAIGHT]);
    track_edge* e4 = &(e2->dest->edge[DIR_CURVED]);
    track_edge* e5 = e3->reverse;
    track_edge* e6 = e4->reverse;

    if (canReserve(e1, trainNum) && canReserve(e2, trainNum) &&
        canReserve(e3, trainNum) && canReserve(e4, trainNum) &&
        canReserve(e5, trainNum) && canReserve(e6, trainNum)) {
#ifdef DEBUG_RESERVATION
      PrintDebug(ui, "%d Merge.. %s %s %s %s %s %s %s %s %s %s %s %s",
          trainNum, e1->src->name, e1->dest->name, e2->src->name, e2->dest->name, e3->src->name,
          e3->dest->name, e4->src->name, e4->dest->name, e5->src->name, e5->dest->name,
          e6->src->name, e6->dest->name);
#endif
      // Center stuff, dont support EX1, EX2
      if (e3->dest->num > 100 &&
          canReserve(&(e3->dest->edge[DIR_CURVED]), trainNum)) {
        e3->dest->edge[DIR_CURVED].reserved_train_num = trainNum;
        e3->dest->edge[DIR_STRAIGHT].reserved_train_num = trainNum;
        e3->dest->edge[DIR_CURVED].reverse->reserved_train_num = trainNum;
        e3->dest->edge[DIR_STRAIGHT].reverse->reserved_train_num = trainNum;
      }

      e1->reserved_train_num = trainNum;
      e2->reserved_train_num = trainNum;
      e3->reserved_train_num = trainNum;
      e4->reserved_train_num = trainNum;
      e5->reserved_train_num = trainNum;
      e6->reserved_train_num = trainNum;

      int tmpStopping = stoppingDistance - e1->dist;
      if (e1->dest->type ==  NODE_BRANCH || e1->dest->type == NODE_MERGE
          || tmpStopping > 0) {
        status = reserveEdges(e1->dest, trainNum, tmpStopping, dryrun);
      }
      if (status == RESERVE_FAIL || dryrun) {
        e1->reserved_train_num = UNRESERVED;
        e2->reserved_train_num = UNRESERVED;
        e3->reserved_train_num = UNRESERVED;
        e4->reserved_train_num = UNRESERVED;
        e5->reserved_train_num = UNRESERVED;
        e6->reserved_train_num = UNRESERVED;
        if (e3->dest->num > 100) {
          e3->dest->edge[DIR_CURVED].reserved_train_num = UNRESERVED;
          e3->dest->edge[DIR_STRAIGHT].reserved_train_num = UNRESERVED;
          e3->dest->edge[DIR_CURVED].reverse->reserved_train_num = UNRESERVED;
          e3->dest->edge[DIR_STRAIGHT].reverse->reserved_train_num = UNRESERVED;
        }
      }
    } else {
#ifdef DEBUG_RESERVATION
      PrintDebug(ui, "%d Reserved switch edge %s FAIL",
          e1->reserved_train_num, node->name);
#endif
      return RESERVE_FAIL;
    }
  } else {
#ifdef DEBUG_RESERVATION
    PrintDebug(ui, "Canot reserve edge type: %d %d", node->type, node->name);
#endif
    return RESERVE_FAIL;
  }
  return status;
}

static void fakeNode(track_edge* edge, track_node* fake1, track_node* fake2, int offset) {
  track_edge* edgeReverse = edge->reverse;
  int dirType;
  int dirTypeReverse;

  dirTypeReverse = edgeType(edgeReverse);
  dirType = edgeType(edge);

  fake1->name = "Fake";
  fake1->type = NODE_FAKE;
  fake1->reverse = fake2;
  fake1->edge[DIR_AHEAD].reverse = &edge->dest->reverse->edge[dirTypeReverse];
  fake1->edge[DIR_AHEAD].src = fake1;
  fake1->edge[DIR_AHEAD].dest = edge->dest;
  fake1->edge[DIR_AHEAD].dist = edge->dist - offset;

  fake2->name = "Reverse Fake";
  fake2->type = NODE_FAKE;
  fake2->reverse = fake1;
  fake2->edge[DIR_AHEAD].reverse = &edge->src->reverse->edge[dirType];
  fake2->edge[DIR_AHEAD].src = fake2;
  fake2->edge[DIR_AHEAD].dest = edgeReverse->dest;
  fake2->edge[DIR_AHEAD].dist = offset;

  edge->dest->reverse->edge[dirTypeReverse].reverse = &fake1->edge[DIR_AHEAD];
  edge->dest->reverse->edge[dirTypeReverse].dest = fake2;
  edge->dest->reverse->edge[dirTypeReverse].dist = edge->dist -offset;

  edge->src->edge[dirType].reverse = &fake2->edge[DIR_AHEAD];
  edge->src->edge[dirType].dest = fake1;
  edge->src->edge[dirType].dist = offset;
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

// bad use of globals
int secondaryPredictionCount;

// Oh my, so many parameters
static void findNextSensorsHelper(track_node* currentNode, TrackSensorPrediction* predictions, int trainNumber, int distance, int error, TrackLandmark errorLandmark, int errorCondition) {
  if (currentNode->type == NODE_SENSOR || currentNode->type == NODE_EXIT) {
    if (error) {
      predictions[1 + secondaryPredictionCount].sensor = getLandmark(currentNode);
      predictions[1 + secondaryPredictionCount].dist = distance;
      predictions[1 + secondaryPredictionCount].conditionLandmark = errorLandmark;
      predictions[1 + secondaryPredictionCount].condition = errorCondition;
      secondaryPredictionCount++;
    } else {
      predictions[0].sensor = getLandmark(currentNode);
      predictions[0].dist = distance;

      // Primary Sensor failure
      if (currentNode->type == NODE_SENSOR) {
        track_edge *edge = &currentNode->edge[DIR_AHEAD];
        findNextSensorsHelper(edge->dest, predictions, trainNumber, distance + edge->dist, 1, getLandmark(currentNode), -1);
      }
    }
  } else if (currentNode->type == NODE_BRANCH) {
    int switch_num = currentNode->num;
    if (switchStatus[switch_num] ==  SWITCH_CURVED) {
      track_edge* edge = &currentNode->edge[DIR_CURVED];
      findNextSensorsHelper(edge->dest, predictions, trainNumber, distance + edge->dist, error, errorLandmark, errorCondition);

      // handle failed switch
      if (!error) {
        edge = &currentNode->edge[DIR_STRAIGHT];
        findNextSensorsHelper(edge->dest, predictions, trainNumber, distance + edge->dist, 1, getLandmark(currentNode), SWITCH_CURVED);
      }
    } else {
      track_edge* edge = &currentNode->edge[DIR_STRAIGHT];
      findNextSensorsHelper(edge->dest, predictions, trainNumber, distance + edge->dist, error, errorLandmark, errorCondition);

      // handle failed switch
      if (!error) {
        edge = &currentNode->edge[DIR_CURVED];
        findNextSensorsHelper(edge->dest, predictions, trainNumber, distance + edge->dist, 1, getLandmark(currentNode), SWITCH_CURVED);
      }
    }
  } else {
    track_edge *edge = &currentNode->edge[DIR_AHEAD];
    findNextSensorsHelper(edge->dest, predictions, trainNumber, distance + edge->dist, error, errorLandmark, errorCondition);
  }
}

static int findNextSensors(track_node *track, Position pos, TrackSensorPrediction* predictions, int trainNumber) {
  track_edge* fromEdge = (track_edge*)NULL;
  int offset = locateNode(track, pos, &fromEdge);
  if (offset == -1) {
    return -1;
  }

  track_node* nodeSrcPointer = fromEdge->src;
  track_node nodeSrc = *fromEdge->src;
  track_node* nodeReverseSrcPointer = fromEdge->reverse->src;
  track_node nodeReverseSrc = *fromEdge->reverse->src;

  fakeNode(fromEdge, &track[TRACK_MAX], &track[TRACK_MAX + 1], offset);

  track_node* currentNode = &track[TRACK_MAX];

  // bad use of globals
  secondaryPredictionCount = 0;
  TrackLandmark fakeLandmark = {LANDMARK_FAKE, 0, 0};
  findNextSensorsHelper(currentNode, predictions, trainNumber, 0/*distance*/, 0/*error*/, fakeLandmark, 0/*errorCondition*/);

  // restore graph
  *nodeSrcPointer = nodeSrc;
  *nodeReverseSrcPointer = nodeReverseSrc;
  return 1 + secondaryPredictionCount; // 1 primary + secondaryPredictionCount secondary
}

static void computeSafeReverseDistHelper(track_edge* edge) {
  if (edge->src->safe_reverse_dist == INT_MAX) {
    return;
  }

  int new_safe = edge->src->safe_reverse_dist - edge->dist;
  if (new_safe > 0) {
    edge->src->safe_reverse_dist = INT_MAX;
    edge->dest->safe_reverse_dist = MAX(new_safe, edge->dest->safe_reverse_dist);
    if (edge->dest->type == NODE_BRANCH) {
      computeSafeReverseDistHelper(&edge->dest->edge[DIR_CURVED]);
    } else if (edge->dest->type != NODE_EXIT) {
      computeSafeReverseDistHelper(&edge->dest->edge[DIR_AHEAD]);
    }
  } else {
    if ((edge->dest->type == NODE_BRANCH || edge->dest->type == NODE_MERGE) && edge->dist < SAFE_REVERSE_DIST) {
      edge->src->safe_reverse_dist = INT_MAX;
    }
  }
}

static void computeSafeReverseDist(track_node* track) {
  for (int i = 0 ; i < TRACK_MAX + 4; i++) {
    if (track[i].type != NODE_NONE) {

    }
    if (track[i].type == NODE_BRANCH) {
      track[i].safe_reverse_dist = SAFE_REVERSE_DIST;
      computeSafeReverseDistHelper(&track[i].edge[DIR_STRAIGHT]);
      computeSafeReverseDistHelper(&track[i].edge[DIR_CURVED]);
    } else if (track[i].type == NODE_MERGE) {
      track[i].safe_reverse_dist = SAFE_REVERSE_DIST;
      computeSafeReverseDistHelper(&track[i].edge[DIR_AHEAD]);
    }
  }
}

// Dijkstra's algorithm, currently slow, need a heap for efficiency
static void findRoute(track_node* track, Position from, Position to, Route* result) {
  // fake position into graph
  track_edge* fromEdge = (track_edge*)NULL;
  int offsetFrom = locateNode(track, from, &fromEdge);

  if (offsetFrom == -1) {
    result->dist = 0;
    result->length = 0;
    return;
  }

  track_node* fromNodeSrcPointer = fromEdge->src;
  track_node fromNodeSrc = *fromEdge->src;
  track_node* fromNodeReverseSrcPointer = fromEdge->reverse->src;
  track_node fromNodeReverseSrc = *fromEdge->reverse->src;

  fakeNode(fromEdge, &track[TRACK_MAX], &track[TRACK_MAX + 1], offsetFrom);

  track_edge* toEdge = (track_edge*)NULL;
  int offsetTo = locateNode(track, to, &toEdge);

  if (offsetTo == -1) {
    result->dist = 0;
    result->length = 0;
    return;
  }

  track_node* toNodeSrcPointer = toEdge->src;
  track_node toNodeSrc = *toEdge->src;
  track_node* toNodeReverseSrcPointer = toEdge->reverse->src;
  track_node toNodeReverseSrc = *toEdge->reverse->src;

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
  char uiName[] = UI_TASK_NAME;
  int ui = -1;
  if (!CALIBRATION) {
    ui = WhoIs(uiName);
  }

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

    for (int i = 0; i < neighbour_count; i++) {
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
      tempRouteNode.num = SWITCH_CURVED;
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
  result->nodes[result->length-1].dist = 0;

  // restore graph
  *toNodeSrcPointer = toNodeSrc;
  *toNodeReverseSrcPointer = toNodeReverseSrc;

  *fromNodeSrcPointer = fromNodeSrc;
  *fromNodeReverseSrcPointer = fromNodeReverseSrc;
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
  UiMsg uimsg;
  ReleaseOldAndReserveNewTrackMsg actualMsg;
  TrackMsg* msg = (TrackMsg*) &actualMsg;

  for ( ;; ) {
    int tid = -1;
    Receive(&tid, (char*)msg, sizeof(TrackMsg));
    switch (msg->type) {
      case RELEASE_OLD_N_RESERVE_NEW: {
        track_node* start = findNode(track, actualMsg.lastSensor);
        char canReserve = reserveEdges(start,
            actualMsg.trainNum, actualMsg.stoppingDistance, 1); // Dryrun
        if (canReserve != RESERVE_SUCESS) {
#ifdef DEBUG_RESERVATION
          PrintDebug(ui, "Train:%d denied reservation %s.",
              actualMsg.trainNum, start->name);
#endif
        } else {
#ifdef DEBUG_RESERVATION
          PrintDebug(ui, "Train:%d got reservation %s.",
              actualMsg.trainNum, start->name);
#endif
          // Clear the train's previous reservation
          for (int j = 0; j < TRACK_MAX +4; j++) {
            clearNodeReservation(&track[j], actualMsg.trainNum);
          }

          // Make current reservation
          canReserve |= reserveEdges(start,
            actualMsg.trainNum, actualMsg.stoppingDistance, 0);
          for (int j = 0; j < actualMsg.numPredSensor; j++) {
            track_node* n = findNode(track, actualMsg.predSensor[j]);
            canReserve |= reserveEdges(n,
                actualMsg.trainNum, actualMsg.stoppingDistance, 0);
          }
        }
        Reply(tid, &canReserve, 1);
        break;
      }
      case SET_SWITCH: {
        Reply(tid, (char*)1, 0);
        TrackLandmark sw = msg->landmark1;
        trackSetSwitch((int)sw.num2, (int)msg->data);
        if (!CALIBRATION) {
          uimsg.type = UPDATE_SWITCH;
          uimsg.data1 = sw.num2;
          uimsg.data2 = msg->data;
          Send(ui, (char*)&uimsg, sizeof(UiMsg), (char*)1, 0);
        }
        break;
      }
      //case QUERY_DISTANCE: {
      //  // TODO
      //  //track_node* from = findNode(track, msg.landmark1);
      //  //track_node* to = findNode(track, msg.landmark2);
      //  //int distance = calculateDistance(from, to, 0);
      //  int distance = 0;
      //  Reply(tid, (char *)&distance, sizeof(int));
      //  break;
      //}
      case QUERY_NEXT_SENSOR_FROM_SENSOR: {
        TrackLandmark sensor =  msg->landmark1;
        track_node* from = findNode(track, sensor);
        TrackLandmark nextLandmark = getLandmark(from->edge[DIR_AHEAD].dest);
        Position pos = {sensor, nextLandmark, 0};

        TrackNextSensorMsg sensorMsg;
        sensorMsg.numPred =
          findNextSensors(track, pos, sensorMsg.predictions, (int)msg->data);

        Reply(tid, (char *)&sensorMsg, sizeof(TrackNextSensorMsg));
        break;
      }
      case QUERY_NEXT_SENSOR_FROM_POS: {
        TrackNextSensorMsg sensorMsg;
        sensorMsg.numPred =
          findNextSensors(track, msg->position1, sensorMsg.predictions, (int)msg->data);

        Reply(tid, (char *)&sensorMsg, sizeof(TrackNextSensorMsg));
        break;
      }
      case ROUTE_PLANNING: {
        Position from = msg->position1;
        Position to = msg->position2;
        Route route;
        findRoute(track, from, to , &route);

        Reply(tid, (char *)&route, 8 + sizeof(RouteNode) * route.length);
        break;
      }
      case SET_TRACK: {
        if (msg->data == 'a') {
          init_tracka(track);
          PrintDebug(ui, "Using Track A");
        } else {
          init_trackb(track);
          PrintDebug(ui, "Using Track B");
        }
        Reply(tid, (char *)NULL, 0);
        break;
      }
      case GET_SWITCH: {
        Reply(tid, (char*)(switchStatus + msg->data), 4);
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

#include <TrackHelper.c>
