#include "CalibrationData.h"

#include <memory.h>

void initMultiTrain(int trainNum, int* trainLen, int* pickupOffset) {
  if (trainNum == 37 || trainNum == 35) {
    *trainLen = 190;    // 190mm
    *pickupOffset = 20; // mm
  } else if (trainNum == 39 ||
             trainNum == 41 ||
             trainNum == 43 ||
             trainNum == 44 ||
             trainNum == 45 ||
             trainNum == 48) {
    *trainLen = 210;    // 210mm
    *pickupOffset = 25; // mm
  } else {
    *trainLen = -1;
    *pickupOffset = -1;
  }
}

void initVelocity(int* velocity, int trainNum) {
  if (trainNum == 37 || trainNum == 35) {
    int v[15][2] = {
      {0.0, 0.0},
      {2080, 2080},
      {6800, 6800},
      {12500, 12500},
      {17444, 17444},
      {22428, 22428},
      {28035, 28035},
      {32730, 32730},
      {37708, 37708},
      {43611, 43611},
      {51986, 57064},
      {55979, 57064},
      {61811, 61811},
      {65966, 64581}
    };
    memcpy_no_overlap_asm((char*)v, (char*)velocity, 2*15*4);

  } else if (trainNum == 39 ||
             trainNum == 41 ||
             trainNum == 44 ||
             trainNum == 48 ||
             trainNum == 45) {
    int v[15][2] = {
      {0.0, 0.0},
      {4000.0, 4000.0},
      {7300.0, 7300.0},
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
  } else if (trainNum == 43) {
    int v[15][2] = {
      {0.0, 0.0},
      {4000, 4000},
      {7300, 7300},
      {12000, 11000},
      {16000, 14000},
      /*5*/{23728, 23728},
      {29035, 29035},
      {35681, 35681},
      {39250.0, 45573.0},
      {46176, 43611},
      {49011, 49011},
      {55979, 57064},
      {61811, 61811},
      {65966, 64581}
    };
    memcpy_no_overlap_asm((char*)v, (char*)velocity, 2*15*4);
  } else {
    int v[15][2] = {
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0},
      {0, 0}
    };
    memcpy_no_overlap_asm((char*)v, (char*)velocity, 2*15*4);

  }
} // end initVelocity


void initStoppingDistance(int* distance, int trainNum) {
  if (trainNum == 35 || trainNum == 37) {
    // 15 speeds each with Acc/Desc .. each with min/max
    int d[15][2][2] = {
      {{0, 0}, {0, 0} },
      {{10, 15}, {20, 20} },
      {{25, 30}, {35, 40} },
      {{40, 55}, {65, 75} },
      {{75, 90}, {95, 110} },
      {{120, 120}, {120, 120} },
      {{190, 190}, {190, 190} },
      {{270, 270}, {270, 270} },
      {{375, 375}, {375, 375} },
      {{485, 485}, {485, 485} },
      {{595, 595}, {595, 595} },
      {{720, 720}, {720, 720} },
      {{785, 795}, {780, 780} },
      {{960, 962}, {856, 889} },
    };
    memcpy_no_overlap_asm((char*)d, (char*)distance, 2*2*15*4);

  }
  else if (trainNum == 39 ||
           trainNum == 41 ||
           trainNum == 44 ||
           trainNum == 48 ||
           trainNum == 45 ) {
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
      {{1206, 1266}, {1206, 1206} }
    };
    memcpy_no_overlap_asm((char*)d, (char*)distance, 2*2*15*4);
  } else if (trainNum == 43) {
    // 15 speeds each with Acc/Desc .. each with min/max
    int d[15][2][2] = {
      {{0, 0}, {0, 0} },
      {{10, 15}, {20, 20} },
      {{25, 30}, {35, 40} },
      {{40, 55}, {65, 75} },
      {{75, 90}, {95, 110} },
      {{290, 290}, {290, 290} },
      {{400, 400}, {400, 400} },
      {{490, 490}, {490, 490} },
      {{540, 540}, {540, 540} },
      {{630, 630}, {630, 630} },
      {{695, 695}, {695, 695} },
      {{790, 790}, {790, 790} },
      {{860, 860}, {860, 860} },
      {{960, 962}, {856, 889} },
      {{1206, 1266}, {1206, 1206} }
    };
    memcpy_no_overlap_asm((char*)d, (char*)distance, 2*2*15*4);
  } else {
    // 15 speeds each with Acc/Desc .. each with min/max
    int d[15][2][2] = {
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} },
      {{0, 0}, {0, 0} }};
    memcpy_no_overlap_asm((char*)d, (char*)distance, 2*2*15*4);
  }
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

