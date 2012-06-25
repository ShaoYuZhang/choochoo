#ifndef TRAIN_H_
#define TRAIN_H_

#define TRAIN_CONTROLLER_NAME "TRAIN_CTRL\0"
#define NUM_TRAINS 80

#define SET_SPEED  0
#define GET_SPEED  1
#define GOTO_DEST  2
#define UPDATE_UI  3
#define WORKER     4

typedef struct DriverMsg {
  char type;           // As defined above
  char trainNum;       // Train number
  signed char data2;   // Speed (-1 for reverse)
  unsigned char data3; // Delay num, or msg came from worker
  char replyTid;       // The user that first send the message.
} DriverMsg;


int startDriverControllerTask();

#endif // TRAIN_H_
