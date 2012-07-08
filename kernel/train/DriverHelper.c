
static void printLandmark(Driver* me, TrackLandmark* l) {
  if (l->type == LANDMARK_SENSOR) {
    TrainDebug(me, "Landmark Sn  %c%d D:%d",
        'A' +l->num1, l->num2);
  } else if (l->type == LANDMARK_END) {
    TrainDebug(me, "Landmark %s %d D:%d",
        l->num1 == EN ? "EN" : "EX",
        l->num2);
  } else if (l->type == LANDMARK_FAKE) {
    TrainDebug(me, "Landmark Fake %d %d D:%d",
        l->num1, l->num2);
  }
}
