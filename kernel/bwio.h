/*
 * bwio.h
 */
#ifndef BWIO_H_
#define BWIO_H_

#include <util.h>

void uart_stopbits(int channel, int bits);

void uart_databits(int channel, int wlen);

void uart_parity(int channel, int enable);

int bwsetfifo( int channel, int state );

int bwsetspeed( int channel, int speed );

int bwputc( int channel, char c );

int bwgetc( int channel );

int bwputx( int channel, char c );

int bwputr( int channel, unsigned int reg );

int bwputstr( int channel, char *str );

void bwputw( int channel, int n, char fc, char *bf );

void bwprintf( int channel, char *format, ... );

#endif // BWIO_H_
