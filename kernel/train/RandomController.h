#ifndef RANDOM_CONTROLLER_
#define RANDOM_CONTROLLER_

#define RANDOM_CONTROL_NAME "RAND_CK\0"

int startRandomTrainControllerTask();

void startRandom(int trainNum1, int trainNum2);

void stopRandom(int trainNum1, int trainNum2);

#endif //RANDOM_CONTROLLER_
