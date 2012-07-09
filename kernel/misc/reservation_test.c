// Test reserving message.
    Delay(200, time);
  {
    PrintDebug(ui, "Average TEst------");
    TrackMsg rmsg;
    rmsg.type = RESERVE_EDGE;
    rmsg.data = 44; // train num
    rmsg.landmark1.type = LANDMARK_SENSOR;
    rmsg.stoppingDistance = 300;

    TrackMsg cmsg;
    cmsg.type = RESERVE_EDGE;
    cmsg.data = 43; // train num
    cmsg.landmark1.type = LANDMARK_SENSOR;
    cmsg.stoppingDistance = 240;

    char reply;
    rmsg.landmark1.num1 = 3;
    rmsg.landmark1.num2 = 5;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "D5 Good?: %d", reply);

    cmsg.landmark1.num1 = 3;
    cmsg.landmark1.num2 = 5;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "D5 Fail?: %d", reply);

    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 5;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "E5 Fail?: %d", reply);

    rmsg.type = RELEASE_EDGE;
    PrintDebug(ui, "Release", reply);
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);

    cmsg.landmark1.num1 = 3;
    cmsg.landmark1.num2 = 5;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "D5 Good?: %d", reply);

    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 5;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "E5 Good?: %d", reply);
  }
  Delay(100, time);
  {
    PrintDebug(ui, "Branch Switch TEst------");
    TrackMsg rmsg;
    rmsg.type = RESERVE_EDGE;
    rmsg.data = 44; // train num
    rmsg.landmark1.type = LANDMARK_SENSOR;
    rmsg.stoppingDistance = 300;

    TrackMsg cmsg;
    cmsg.type = RESERVE_EDGE;
    cmsg.data = 43; // train num
    cmsg.landmark1.type = LANDMARK_SENSOR;
    cmsg.stoppingDistance = 100;

    char reply;
    rmsg.landmark1.num1 = 4;
    rmsg.landmark1.num2 = 6;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "E6 Good?: %d", reply);
    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 6;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "E6 Fail?: %d", reply);
    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 4;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "E4 Fail?: %d", reply);
    cmsg.landmark1.num1 = 3;
    cmsg.landmark1.num2 = 3;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "D3 Fail?: %d", reply);
    cmsg.landmark1.num1 = 3;
    cmsg.landmark1.num2 = 4;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "D4 Good?: %d", reply);
    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 3;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "E3 Good?: %d", reply);


    rmsg.type = RELEASE_EDGE;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);

    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 6;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "Good?: %d", reply);
    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 4;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "Good?: %d", reply);
    cmsg.landmark1.num1 = 3;
    cmsg.landmark1.num2 = 3;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "Good?: %d", reply);
    cmsg.landmark1.num1 = 3;
    cmsg.landmark1.num2 = 4;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "Good?: %d", reply);
    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 3;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "Good?: %d", reply);
  }
  Delay(60, time);
  {
    PrintDebug(ui, "Merge Switch Test------");
    TrackMsg rmsg;
    rmsg.type = RESERVE_EDGE;
    rmsg.data = 44; // train num
    rmsg.landmark1.type = LANDMARK_SENSOR;
    rmsg.stoppingDistance = 350;

    TrackMsg cmsg;
    cmsg.type = RESERVE_EDGE;
    cmsg.data = 43; // train num
    cmsg.landmark1.type = LANDMARK_SENSOR;
    cmsg.stoppingDistance = 1;

    char reply;
    rmsg.landmark1.num1 = 2;
    rmsg.landmark1.num2 = 12;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C12 Success?: %d", reply);

    cmsg.landmark1.num1 = 2;
    cmsg.landmark1.num2 = 12;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C12 Fail?: %d", reply);

    cmsg.landmark1.num1 = 2;
    cmsg.landmark1.num2 = 14;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C14 fail?: %d", reply); // Because after c14 is a switch, need to reserve switch.

    cmsg.landmark1.num1 = 2;
    cmsg.landmark1.num2 = 13;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C13 Good?: %d", reply);

    cmsg.landmark1.num1 = 0;
    cmsg.landmark1.num2 = 3;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "A3 Fail?: %d", reply);
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);

    cmsg.landmark1.num1 = 0;
    cmsg.landmark1.num2 = 4;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "A4 Good?: %d", reply);
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);

    cmsg.landmark1.type = LANDMARK_SWITCH;
    cmsg.landmark1.num1 = MR;
    cmsg.landmark1.num2 = 11;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "MR 11 Fail?: %d", reply);

    rmsg.type = RELEASE_EDGE;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);

    PrintDebug(ui, "-----------");
    cmsg.landmark1.type = LANDMARK_SENSOR;
    cmsg.landmark1.num1 = 2;
    cmsg.landmark1.num2 = 12;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C12 Good?: %d", reply);

    cmsg.landmark1.num1 = 2;
    cmsg.landmark1.num2 = 14;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C14 Good?: %d", reply);

    cmsg.landmark1.num1 = 2;
    cmsg.landmark1.num2 = 13;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C13 Good?: %d", reply);

    cmsg.landmark1.num1 = 0;
    cmsg.landmark1.num2 = 3;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "A3 Fail?: %d", reply);
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);

    cmsg.landmark1.num1 = 0;
    cmsg.landmark1.num2 = 4;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "A4 Good?: %d", reply);
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);

    cmsg.landmark1.type = LANDMARK_SWITCH;
    cmsg.landmark1.num1 = MR;
    cmsg.landmark1.num2 = 11;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "MR 11 Good?: %d", reply);
  }

  Delay(100, time);
  {
    PrintDebug(ui, "Center Switch Test------");
    TrackMsg rmsg;
    rmsg.type = RESERVE_EDGE;
    rmsg.data = 44; // train num
    rmsg.landmark1.type = LANDMARK_SENSOR;
    rmsg.stoppingDistance = 100;

    TrackMsg cmsg;
    cmsg.type = RESERVE_EDGE;
    cmsg.data = 43; // train num
    cmsg.landmark1.type = LANDMARK_SENSOR;
    cmsg.stoppingDistance = 100;

    char reply;
    rmsg.landmark1.num1 = 1;
    rmsg.landmark1.num2 = 13;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "B13 Good?: %d", reply);

    cmsg.landmark1.num1 = 1;
    cmsg.landmark1.num2 = 13;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "B13 Fail?: %d", reply);

    cmsg.landmark1.num1 = 3;
    cmsg.landmark1.num2 = 1;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "D1 Fail?: %d", reply);

    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 1;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "E1 Fail?: %d", reply);

    cmsg.landmark1.num1 = 2;
    cmsg.landmark1.num2 = 2;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C2 Fail?: %d", reply);
    rmsg.type = RELEASE_EDGE;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);

    cmsg.landmark1.num1 = 1;
    cmsg.landmark1.num2 = 13;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "B13 good?: %d", reply);

    cmsg.landmark1.num1 = 3;
    cmsg.landmark1.num2 = 1;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "D1 good?: %d", reply);

    cmsg.landmark1.num1 = 4;
    cmsg.landmark1.num2 = 1;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "E1 good?: %d", reply);

    cmsg.landmark1.num1 = 2;
    cmsg.landmark1.num2 = 2;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "C2 good?: %d", reply);
  }

  Delay(100, time);
  {
    PrintDebug(ui, "Terminal Test------");
    TrackMsg rmsg;
    rmsg.type = RESERVE_EDGE;
    rmsg.data = 44; // train num
    rmsg.landmark1.type = LANDMARK_SENSOR;
    rmsg.stoppingDistance = 30;

    TrackMsg cmsg;
    cmsg.type = RESERVE_EDGE;
    cmsg.data = 43; // train num
    cmsg.landmark1.type = LANDMARK_SENSOR;
    cmsg.stoppingDistance = 30;

    char reply;
    rmsg.landmark1.num1 = 0;
    rmsg.landmark1.num2 = 2;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "A2 Good?: %d", reply);

    cmsg.landmark1.num1 = 0;
    cmsg.landmark1.num2 = 2;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "A2 fail?: %d", reply);

    cmsg.landmark1.type = LANDMARK_END;
    cmsg.landmark1.num1 = EN;
    cmsg.landmark1.num2 = 5;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "EN5 fail?: %d", reply);
    rmsg.type = RELEASE_EDGE;
    Send(trackController, (char*)&rmsg, sizeof(TrackMsg), &reply, 1);

    cmsg.landmark1.num1 = 0;
    cmsg.landmark1.num2 = 2;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "A2 good?: %d", reply);

    cmsg.landmark1.type = LANDMARK_END;
    cmsg.landmark1.num1 = EN;
    cmsg.landmark1.num2 = 5;
    Send(trackController, (char*)&cmsg, sizeof(TrackMsg), &reply, 1);
    PrintDebug(ui, "EN5 good?: %d", reply);
  }


