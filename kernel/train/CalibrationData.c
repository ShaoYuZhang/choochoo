#include "CalibrationData.h"

#include <memory.h>


void initVelocity(int* velocity) {
  int v[15][2] = {
{0.0, 0.0},
{4000.0, 4000.0},
{8000.0, 8000.0},
{12000.0, 11000.0},
{16000.0, 14000.0},
{22428.0, 28136.0},
{28035.0, 34388.0},
{34130.0, 39447.0},
{39250.0, 45573.0},
{43611.0, 51986.0},
{51986.0, 57064.0},
{55979.0, 57064.0},
{61811.0, 61811.0},
{65966.0, 64581.0}
};
memcpy_no_overlap_asm((char*)v, (char*)velocity, 2*15*4);
} // end initVelocity


void initStoppingDistance(int* distance) {
  // 15 speeds each with Acc/Desc .. each with min/max
  int d[15][2][2] = {
{{0, 0}, {0, 0} },
{{10, 15}, {20, 20} },
{{25, 30}, {35, 40} },
{{40, 55}, {65, 75} },
{{75, 90}, {95, 110} },
{{240, 240}, {240, 240} },
{{320, 320}, {320, 320} },
{{400, 400}, {400, 400} },
{{440, 440}, {425, 427} },
{{470, 470}, {440, 460} },
{{585, 609}, {585, 600} },
{{785, 795}, {715, 730} },
{{960, 962}, {856, 889} },
{{1206, 1266}, {1206, 1206} },
};
memcpy_no_overlap_asm((char*)d, (char*)distance, 2*2*15*4);
} // end initDistance


// Num millisecond to accelerate to speed from zero.
void initAccelerationProfile(int* accel) {
  int a[15] = {
       0
      ,700
      ,1200
      ,2000
      ,2700
      ,3194
      ,3486
      ,3979
      ,4274
      ,4608
      ,4779
      ,4995
      ,5260
      ,5500
      ,5800
  };

  memcpy_no_overlap_asm((char*)a, (char*)accel, 15*4);
}

