static void initPresetRoute1(Route *route) {
  int index = 0;
  {
    RouteNode node;
    node.landmark.type = LANDMARK_FAKE;
    node.landmark.num1 = 0;
    node.landmark.num2 = 0;
    node.dist = 200;
    route->nodes[index++] = node;
  }
  // go in loops for n times
  for (int i = 0; i < 6; i++) {
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 1;
      node.landmark.num2 = 16;
      node.dist = 50;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = BR;
      node.landmark.num2 = 15;
      node.num = SWITCH_CURVED;
      node.dist = 326;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 2;
      node.landmark.num2 = 10;
      node.dist = 128;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = BR;
      node.landmark.num2 = 16;
      node.num = SWITCH_CURVED;
      node.dist = 239;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 1;
      node.landmark.num2 = 3;
      node.dist = 201;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 2;
      node.landmark.num2 = 2;
      node.dist = 246;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = MR;
      node.landmark.num2 = 153;
      node.dist = 0;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = MR;
      node.landmark.num2 = 154;
      node.dist = 0;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = BR;
      node.landmark.num2 = 156;
      node.num = SWITCH_CURVED;
      node.dist = 0;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 4;
      node.landmark.num2 = 2;
      node.dist = 201;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 4;
      node.landmark.num2 = 15;
      node.dist = 246;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = MR;
      node.landmark.num2 = 13;
      node.dist = 120;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 3;
      node.landmark.num2 = 12;
      node.dist = 333;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      // this is actually MR, but i need this to be BR
      // to make it loop properly, or else the train get
      // just set every switch on the first try
      node.landmark.num1 = BR;
      node.landmark.num2 = 14;
      node.num = SWITCH_CURVED;
      node.dist = 43;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 0;
      node.landmark.num2 = 4;
      node.dist = 437;
      route->nodes[index++] = node;
    }
  } // for
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SENSOR;
    node.landmark.num1 = 1;
    node.landmark.num2 = 16;
    node.dist = 50;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SWITCH;
    node.landmark.num1 = BR;
    node.landmark.num2 = 15;
    node.num = SWITCH_STRAIGHT;
    node.dist = 433;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SENSOR;
    node.landmark.num1 = 2;
    node.landmark.num2 = 5;
    node.dist = 61;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SWITCH;
    node.landmark.num1 = BR;
    node.landmark.num2 = 6;
    node.num = SWITCH_STRAIGHT;
    node.dist = 239;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SENSOR;
    node.landmark.num1 = 2;
    node.landmark.num2 = 15;
    node.dist = 300;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_FAKE;
    node.landmark.num1 = 0;
    node.landmark.num2 = 0;
    node.dist = 0;
    route->nodes[index++] = node;
  }
  route->length = index;
  route->dist = 1000000; // not really useful
}

static void initPresetRoute2(Route *route) {
  int index = 0;
  {
    RouteNode node;
    node.landmark.type = LANDMARK_FAKE;
    node.landmark.num1 = 0;
    node.landmark.num2 = 0;
    node.dist = 200;
    route->nodes[index++] = node;
  }
  // go in loops for n times
  for (int i = 0; i < 6; i++) {
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = MR;
      node.landmark.num2 = 9;
      node.dist = 155;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = BR;
      node.landmark.num2 = 8;
      node.num = SWITCH_CURVED;
      node.dist = 239;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 4;
      node.landmark.num2 = 10;
      node.dist = 282;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 4;
      node.landmark.num2 = 13;
      node.dist = 43;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = BR;
      node.landmark.num2 = 17;
      node.num = SWITCH_CURVED;
      node.dist = 246;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 3;
      node.landmark.num2 = 15;
      node.dist = 201;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 1;
      node.landmark.num2 = 13;
      node.dist = 239;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = MR;
      node.landmark.num2 = 154;
      node.dist = 0;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = MR;
      node.landmark.num2 = 154;
      node.dist = 0;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = BR;
      node.landmark.num2 = 156;
      node.num = SWITCH_STRAIGHT;
      node.dist = 0;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      node.landmark.num1 = BR;
      node.landmark.num2 = 155;
      node.num = SWITCH_CURVED;
      node.dist = 246;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 3;
      node.landmark.num2 = 2;
      node.dist = 201;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 4;
      node.landmark.num2 = 4;
      node.dist = 239;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SWITCH;
      // hack, look at comment above for more detail
      node.landmark.num1 = BR;
      node.landmark.num2 = 10;
      node.num = SWITCH_CURVED;
      node.dist = 50;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 4;
      node.landmark.num2 = 5;
      node.dist = 282;
      route->nodes[index++] = node;
    }
    {
      RouteNode node;
      node.landmark.type = LANDMARK_SENSOR;
      node.landmark.num1 = 3;
      node.landmark.num2 = 6;
      node.dist = 229;
      route->nodes[index++] = node;
    }
  } // for
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SWITCH;
    node.landmark.num1 = MR;
    node.landmark.num2 = 9;
    node.dist = 155;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SWITCH;
    node.landmark.num1 = BR;
    node.landmark.num2 = 8;
    node.num = SWITCH_CURVED;
    node.dist = 239;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SENSOR;
    node.landmark.num1 = 4;
    node.landmark.num2 = 10;
    node.dist = 282;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SENSOR;
    node.landmark.num1 = 4;
    node.landmark.num2 = 13;
    node.dist = 43;
    route->nodes[index++] = node;
  }

  {
    RouteNode node;
    node.landmark.type = LANDMARK_SWITCH;
    node.landmark.num1 = BR;
    node.landmark.num2 = 17;
    node.num = SWITCH_STRAIGHT;
    node.dist = 239;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SENSOR;
    node.landmark.num1 = 3;
    node.landmark.num2 = 13;
    node.dist = 404;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_SENSOR;
    node.landmark.num1 = 1;
    node.landmark.num2 = 2;
    node.dist = 50;
    route->nodes[index++] = node;
  }
  {
    RouteNode node;
    node.landmark.type = LANDMARK_FAKE;
    node.landmark.num1 = 0;
    node.landmark.num2 = 0;
    node.dist = 0;
    route->nodes[index++] = node;
  }
  route->length = index;
  route->dist = 1000000; // not really useful
}
