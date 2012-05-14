CC = gcc
AS	= as
CFLAGS  = -c -fPIC -Wall -I. -I../include -mcpu=arm920t -msoft-float -MMD
LDFLAGS = -init main -Map main.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2
ASFLAGS	= -mcpu=arm920t -mapcs-32

%.s: %.c
	$(CC) -S $(CFLAGS) $*.c

%.o: %.c
	$(CC) $(CFLAGS) $*.c

OBJECTS = main.o bwio.o
DEPENDS = ${OBJECTS:.o=.d} # substitute ".o" with ".d"
ASMS = $(OBJECTS:.o=.s)

all: $(ASMS) main.elf 
	cp main.elf /u/cs452/public/tftp/ARM/r4cao/

main.elf: $(OBJECTS) 
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) -lgcc

clean:
	-rm -f *.s *.a *.o *.map *.elf *.d

-include ${DEPENDS} # reads the .d files and reruns dependencies
