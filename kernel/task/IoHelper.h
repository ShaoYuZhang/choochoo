#ifndef IOHELPER_H_
#define IOHELPER_H_

#include <util.h>

void putx( int tid, char c );

void putr( int tid, unsigned int reg );

void putw( int tid, int n, char fc, char *bf );

void printff( int tid, char *format, ... );

#endif // IOHELPER_H_
