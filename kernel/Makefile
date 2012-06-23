CC = gcc
AS = as
AR = ar
LD = ld

# CANNOT enalbe finline-function due to syscall file messing up.
CFLAGS  = -c -fPIC -Wall -I. -Itask/. -Itrain/. -Itrack/ -mcpu=arm920t -msoft-float -MMD -std=c99 -O3
LDFLAGS = -init main -Map main.map -N -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 
ASFLAGS	= -mcpu=arm920t -mapcs-32 -mno-fpu

%.s: %.c
	$(CC) -S $(CFLAGS) $*.c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $*.c -o $@ $<

OBJECTS = bwio.o memory.o kernel.o syscall.o Scheduler.o TaskQueue.o \
					task/NameServer.o task/idle.o task/TimeServer.o task/IoServer.o task/IoBuffer.o task/IoHelper.o task/UserInterface.o \
					train/Train.o train/CommandDecoder.o train/Sensor.o track/Track.o track/track_data.o
DEPENDS = ${OBJECTS:.o=.d} # substitute ".o" with ".d"
ASMS = $(OBJECTS:.o=.s) main.s

all: $(ASMS) main.elf
	cp main.elf /u/cs452/public/tftp/ARM/${USER}_m.elf
	chmod 777 /u/cs452/public/tftp/ARM/${USER}_m.elf

cali: calibration.elf 
	cp calibration.elf /u/cs452/public/tftp/ARM/${USER}_c.elf
	chmod 777 /u/cs452/public/tftp/ARM/${USER}_c.elf

task/idle_asm.o : task/idle.asm
	$(AS) $(ASFLAGS) -o $@ $<

interrupt.o : interrupt.asm
	$(AS) $(ASFLAGS) -o $@ $<

main.elf: $(OBJECTS) main.o interrupt.o task/idle_asm.o orex.ld 
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) main.o interrupt.o task/idle_asm.o -lgcc

calibration.elf: $(OBJECTS) calibration.o interrupt.o task/idle_asm.o orex.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) calibration.o interrupt.o task/idle_asm.o -lgcc

clean:
	-rm -f *.s */*.s *.o */*.o *.map *.elf */*.d *.d /u/cs452/public/tftp/ARM/${USER}_m.elf /u/cs452/public/tftp/ARM/${USER}_c.elf

-include ${DEPENDS} # reads the .d files and reruns dependencies
