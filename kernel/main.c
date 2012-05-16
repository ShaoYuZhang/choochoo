#include <ts7200.h>
#include <bwio.h>
#include <TaskDescriptor.h>
#include <Scheduler.h>

void initialize();
void kerxit(void* taskDescriptor, void* request);
void kerent();
extern char __end;
extern char* freeMemoryBegin;

int main( int argc, char *argv[] ) {
  // Assign the start of free memory
  // (so we can start initializations that require memory.)
  freeMemoryBegin = &__end + 2; // Add two because __end is used.
  int diff = ((unsigned int)freeMemoryBegin)%4;
  if (diff != 0){
    freeMemoryBegin  += (4-diff);
  }

  bwsetfifo( COM2, OFF );
  bwsetspeed( COM2, 115200);

  // TODO declare kernel data structures
  //initialize(); // tds is an array of TDs

  for (int i = 0; i < 4; i++ ) {

    // TODO
    TaskDescriptor* active = schedule();

    kerxit( NULL, NULL); // req is a pointer to a Request

    // TODO
    //handle( tds, req );
  }
}

void initialize() {
}

void kerxit(void* taskDescriptor, void* request) {
  bwprintf(COM2, "kerxit.c: Hello.\n\r" );
  bwprintf(COM2, "kerxit.c: Activating.\n\r" );

  kerent();
  bwprintf(COM2, "kerxit.c: Good-bye.\n\r" );
}


void  kerent(){
}

