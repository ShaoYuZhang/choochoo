static int get_node_type(TrackLandmark landmark) {
  int type = NODE_NONE;
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
      PrintDebug(ui, "Not suppported Landmark type.");
    }
  }
  return type;
}

static int get_node_num(TrackLandmark landmark) {
  int num = -1;
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
      PrintDebug(ui, "Not suppported Landmark type.");
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
    PrintDebug(ui, "invalid track node__________getLandmark");
    landmark.type = 0;
    landmark.num1 = 0;
    landmark.num2 = 0;
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
  PrintDebug(ui, "WARNING: can't find node %d %d %d, FIXME NOW", landmark.type, landmark.num1, landmark.num2);
  return (track_node*) NULL;
}

// non-generic,only support landmarks that are relatively close
// also can't have sensors in between because that cause ambiguity
static int traverse(track_node* currentNode, track_node* targetNode, int depth, int max_depth, track_edge** edges) {
  if (currentNode == targetNode) {
    return depth;
  }
  if (depth == max_depth) {
    return -1;
  }

  if (currentNode->type == NODE_EXIT) {
    return -1;
  } else if (currentNode->type == NODE_SENSOR && depth != 0) {
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
      *edges = edge2;
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
    // for robusteness, when offset less than zero, assume position is on top of landmark1
    if (dist <= 0) {
      *edge = edges[0];
      return 0;
    }
    for (int i = 0; i < len; i++) {
      if (dist < edges[i]->dist) {
        *edge = edges[i];
        return dist;
      }
      else {
        dist -= edges[i]->dist;
      }
    }
    // for robusteness, when offset is more than distance between landmark1 and landmark2, assume position is on top of landmark2
    if (dist >= 0) {
      *edge = edges[len-1];
      return edges[len-1]->dist;
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

