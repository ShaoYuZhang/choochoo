/*
 * bwio.h
 */
#ifndef BWIO_H_
#define BWIO_H_

#include <util.h>

void bwioInit();

int bwsetfifo( int channel, int state );

int bwsetspeed( int channel, int speed );

int bwputc( int channel, char c );

int bwgetc( int channel );

int bwputstr( int channel, char *str );

void bwputw( int channel, int n, char fc, char *bf );

void bwprintf( int channel, char *format, ... );

#endif // BWIO_H_
