#include <ts7200.h>
#include <bwio.h>

void initialize();
void kerxit(void* taskDescriptor, void* request);

int main( int argc, char *argv[] ) {
  bwsetfifo( COM2, OFF );
  bwsetspeed( COM2, 115200);

  // TODO declare kernel data structures
  //initialize(); // tds is an array of TDs

  for (int i = 0; i < 4; i++ ) {

    // TODO
    //active = schedule( tds );

    kerxit( NULL, NULL); // req is a pointer to a Request

    // TODO
    //handle( tds, req );
  }
}

void initialize() {
}

void kerxit(void* taskDescriptor, void* request) {
}

