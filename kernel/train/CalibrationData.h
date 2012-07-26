#ifndef CALIBRATION_DATA_H_
#define CALIBRATION_DATA_H_

#define ACCELERATE 0
#define DECELERATE 1

#define MIN_VAL 0
#define MAX_VAL 1

#define PICKUP_LEN 50

// Mapping between speed and velocity
// Units: 10*um/s
// Get Velocity based on current speed and whether slowing down or speeding up.
void initVelocity(int* velocity, int trainNum);

// Best estimated velocity
// Units: mm
void initStoppingDistance(int* stoppingDistance, int trainNum);

// Num millisecond to accelerate to speed from zero.
void initAccelerationProfile(int* accel);

void initMultiTrain(int trainNum, int* trainLen, int* pickupOffset);
#endif // CALIBRATION_DATA_H_
