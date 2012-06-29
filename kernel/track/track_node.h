#ifndef TRACK_NODE_H
#define TRACK_NODE_H
typedef enum {
  NODE_NONE,
  NODE_SENSOR,
  NODE_BRANCH,
  NODE_MERGE,
  NODE_ENTER,
  NODE_EXIT,
  NODE_FAKE, // for route planning
} node_type;

#define DIR_AHEAD 0
#define DIR_STRAIGHT 0
#define DIR_CURVED 1

struct track_node;
typedef struct track_node track_node;
typedef struct track_edge track_edge;

struct track_edge {
  track_edge *reverse;
  track_node *src, *dest;
  int dist;             /* in millimetres */
  int curveness;        /* in percentage */
};

struct track_node {
  const char *name;
  node_type type;
  int num;              /* sensor or switch number */
  track_node *reverse;  /* same location, but opposite direction */
  track_edge edge[2];

  int curr_dist; // for route planning
  int in_queue; // for route planning
  track_node* route_previous; // for route planning
  int safe_reverse_dist; // for route planning
};
#endif
