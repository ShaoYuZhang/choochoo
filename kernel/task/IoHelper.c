#include <ts7200.h>
#include <IoHelper.h>
#include <IoServer.h>

static inline char c2x( char ch ) {
	if ( (ch <= 9) ) return '0' + ch;
	return 'a' + ch - 10;
}

void putx( int tid, char c ) {
	char chh, chl;

	chh = c2x( c / 16 );
	chl = c2x( c % 16 );
	Putc( tid, chh );
	Putc( tid, chl );
}

void putr( int tid, unsigned int reg ) {
	int byte;
	char *ch = (char *) &reg;

	for( byte = 3; byte >= 0; byte-- ) putx( tid, ch[byte] );
	Putc( tid, ' ' );
}

void putstr( int tid, char *str ) {
	while( *str ) {
		Putc( tid, *str );
		str++;
	}
}

void putw( int tid, int n, char fc, char *bf ) {
	char ch;
	char *p = bf;

	while( *p++ && n > 0 ) n--;
	while( n-- > 0 ) Putc( tid, fc );
	while( ( ch = *bf++ ) ) Putc( tid, ch );
}

int a2d( char ch ) {
	if( ch >= '0' && ch <= '9' ) return ch - '0';
	if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	return -1;
}

char a2i( char ch, char **src, int base, int *nump ) {
	int num, digit;
	char *p;

	p = *src; num = 0;
	while( ( digit = a2d( ch ) ) >= 0 ) {
		if ( digit > base ) break;
		num = num*base + digit;
		ch = *p++;
	}
	*src = p; *nump = num;
	return ch;
}

void ui2a( unsigned int num, unsigned int base, char *bf ) {
	int n = 0;
	int dgt;
	unsigned int d = 1;

	while( (num / d) >= base ) d *= base;
	while( d != 0 ) {
		dgt = num / d;
		num %= d;
		d /= base;
		if( n || dgt > 0 || d == 0 ) {
			*bf++ = dgt + ( dgt < 10 ? '0' : 'a' - 10 );
			++n;
		}
	}
	*bf = 0;
}

void i2a( int num, char *bf ) {
	if( num < 0 ) {
		num = -num;
		*bf++ = '-';
	}
	ui2a( num, 10, bf );
}

void format ( int tid, char *fmt, va_list va ) {
	char bf[12];
	char ch, lz;
	int w;


	while ( ( ch = *(fmt++) ) ) {
		if ( ch != '%' )
			Putc( tid, ch );
		else {
			lz = 0; w = 0;
			ch = *(fmt++);
			switch ( ch ) {
			case '0':
				lz = 1; ch = *(fmt++);
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				ch = a2i( ch, &fmt, 10, &w );
				break;
			}
			switch( ch ) {
			case 0: return;
			case 'c':
				Putc( tid, va_arg( va, char ) );
				break;
			case 's':
				putw( tid, w, 0, va_arg( va, char* ) );
				break;
			case 'u':
				ui2a( va_arg( va, unsigned int ), 10, bf );
				putw( tid, w, lz, bf );
				break;
			case 'd':
				i2a( va_arg( va, int ), bf );
				putw( tid, w, lz, bf );
				break;
			case 'x':
				ui2a( va_arg( va, unsigned int ), 16, bf );
				putw( tid, w, lz, bf );
				break;
			case '%':
				Putc( tid, ch );
				break;
			}
		}
	}
}

void printff( int tid, char *fmt, ... ) {
  va_list va;

  va_start(va,fmt);
  format( tid, fmt, va );
  va_end(va);
}
