#ifndef CALIBRATION_DATA_H_
#define CALIBRATION_DATA_H_

#define ACCELERATE 0
#define DECELERATE 1

#define MIN_VAL 0
#define MAX_VAL 1

// Mapping between speed and velocity
// Units: 10*um/s
// Get Velocity based on current speed and whether slowing down or speeding up.
void initVelocity(int* velocity);

// Best estimated velocity
// Units: mm
void initStoppingDistance(int* stoppingDistance);

#endif // CALIBRATION_DATA_H_
