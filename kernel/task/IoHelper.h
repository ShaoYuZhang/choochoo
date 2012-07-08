#ifndef IOHELPER_H_
#define IOHELPER_H_

#include <util.h>

void putx( int tid, char c );

void putr( int tid, unsigned int reg );

void putw( int tid, int n, char fc, char *bf );

void printff( int tid, char *fmt, ... );

int sprintff( char *str, char *fmt, ... );

char a2i( char ch, char **src, int base, int *nump );

int strgetui(char **c);
int a2d( char ch );


#endif // IOHELPER_H_
