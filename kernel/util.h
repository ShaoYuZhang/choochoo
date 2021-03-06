#ifndef UTIL_H_
#define UTIL_H_

#include <ts7200.h>
#include <bwio.h>

#define INT_MAX 0x7FFFFFFF

////////// HARDWARE Stuff
#define UART_BASE(_x) (((_x) == COM1) ? UART1_BASE : UART2_BASE)

#define BACKSPACE '\b'
#define RETURN '\r'
#define ESC '\033'

#define ON  1
#define OFF 0

#define NUM_PRIORITY 31 // 0 = HIGHEST, 30 = LOWEST
#define LOWEST_PRIORITY (NUM_PRIORITY - 1)
#define HIGHEST_PRIORITY 0

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

// Also look at the variables in orex.ld
#define USER_MEM_START	0x300000
#define USER_MEM_END	0x1900000
// the size of user memory in bytes (64 KB)
#define STACK_SIZE 65536

// Artificial limit..
#define NUM_MAX_TASK 128

typedef signed char Tid;
#define MASK_HIGHER 0xFFFF0000
#define MASK_LOWER 0xFFFF

////////// TYPES

typedef char* addr;
#define NULL (char*) 1

////////// VAR ARG
typedef char *va_list;

#define __va_argsiz(t)  \
        (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)  ((void)0)

#define va_arg(ap, t)   \
		         (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

///////////// USEFUL MACROS
#define TRUE 1
#define FALSE 0
#define CRLF "\r\n"
#define MEM(x) (*(addr)(x))
#define VMEM(x) (*(volatile unsigned int*)(x))
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define MAX(x, y) ( ( (x) > (y) ) ? (x) : (y) )
#define MIN(x, y) ( ( (x) < (y) ) ? (x) : (y) )
#define INSTALL_INTERRUPT_HANDLER(vec, jmp) { VMEM((vec) + 0x20) = (unsigned int)(jmp); }
#define ROUND_UP(x, num) ((((unsigned int)x)+(num-1))&~(num-1))
// turn mask bits on/off in word based on flag (improve with orr/bic?)
#define BIT_TOGGLE(word, mask, flag) ((word) ^= (-(flag) ^ (word)) & (mask))
#define GET_TIMER4() (*(volatile unsigned int*)(0x80810060))

///////////// DEBUG
#define ASSERT_ENABLED 1
#define MORE_CHECKING  1
#define PERF_CHECK 0

#if ASSERT_ENABLED
#define ASSERT(X, ...) { \
	if (!(X)) { \
    __asm__("mov pc, #1"); \
		printff(COM2, "assertion failed in file " __FILE__ " line:" TOSTRING(__LINE__) CRLF); \
		printff(COM2, "[%s] ", __func__); \
		printff(COM2, __VA_ARGS__); \
		printff(COM2, "\n"); \
		while(1); \
	} \
}
#else
#define ASSERT(X,...)
#endif

#define ERROR(...) { \
	bwprintf(1, "ERROR:" __VA_ARGS__); \
	bwprintf(1, CRLF "File: " __FILE__ " line: " TOSTRING(__LINE__) CRLF); \
	bwprintf(1, "\n"); \
	while(1); \
}

#endif // UTIL_H_
