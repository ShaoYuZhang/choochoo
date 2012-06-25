#ifndef CALIBRATION_DATA_H_
#define CALIBRATION_DATA_H_

#define ACC 0
#define DEC 1

#define MIN 0
#define MAX 1

// Mapping between speed and velocity
// Units: 10*um/s
// TODO: zhang considering, curve related multiplier, train related multiplier

// Get Velocity based on current speed and whether slowing down or speeding up.
void initVelocity(int* velocity);

// Best estimated velocity
// Units: mm
void initStoppingDistance(int* stoppingDistance);

#endif // CALIBRATION_DATA_H_
