# Makefile Winter 2011 
# by
# David Grant
# Thomas Reidemeister
# Irene Huang

DEBUG ?=0
PHASE1 ?=0
PHASE2 ?=0
CC=m68k-elf-gcc
CXX=m68k-elf-g++
BASICFLAGS = -Wall -m5307 -pipe -nostdlib
DEBUGFLAGS= -D_DEBUG
PHASE1FLAGS= -D_DEBUG_HOTKEY
PHASE2FLAGS= -D_AUTOMATIC_TEST_
CFLAGS:= $(BASICFLAGS) 
#$(DEBUGFLAGS) $(PHASE1FLAGS) $(PHASE2FLAGS)

ifeq ($(DEBUG),1)
	CFLAGS += $(DEBUGFLAGS)
endif
ifeq ($(PHASE1),1)
	CFLAGS += $(PHASE1FLAGS)
endif
ifeq ($(PHASE2),1)
	CFLAGS += $(PHASE2FLAGS)
endif


LD=m68k-elf-gcc
AS=m68k-elf-as
AR=m68k-elf-ar
ARFLAGS=
OBJCPY=m68k-elf-objcopy
OBJDUMP=m68k-elf-objdump
ASM=./start.s
RTX_ASM=../trap_handler/trap_handler.s ../scheduler/scheduler.s memset.s ../timer/timer_entry.s ../serial/serial_entry.s
LDFLAGS_RTX = -Trtx.ld -Wl,-Map=rtx.map
LDFLAGS_RTX_LOADER = -Trtx_loader.ld -Wl,-Map=rtx_loader.map
LDFLAGS_RTX_TEST = -Trtx_test.ld -Wl,-Map=rtx_test.map

RTX_OBJS = rtx.o dbug.o main_rtx.o kernel_rtx.o ../memory_pool/MemoryPool.o ../memory_pool/MemoryBlock.o ../message_passing/Mailbox.o ../memory/memory.o ../initialization/ProcessQueue.o ../initialization/Initialization.o ../scheduler/scheduler.o ../trap_handler/trap_handler.o ../initialization/SysProc.o ../keyboard_decoder_process/registered_command.o ../keyboard_decoder_process/keyboard_decoder_process.o ../wall_clock/wall_clock_process.o ../set_process_priority_process/set_process_priority_process.o ../atomic/atomic.o ../serial/serial_interrupt_handler.o ../timer/timer_interrupt_handler.o ../timer/timer0.o ../serial/serial.o ../initialization/UserProc.o ../initialization/ProcessABC.o ../crt_display/crt_display.o

RTX_LOADER_OBJS = dbug.o rtx_loader.o
RTX_TEST_DUMMY_OBJS = dbug.o rtx_test_dummy.o
OST_TESTS_OBJS = dbug.o ../test_process/producer_consumer.o


all: clean mdummy.s19 stevee_dummy.s19

# Note, GCC builds things in order, it's important to put yhe
#  ASM first, so that it is located at the beginning of our program.

rtx.s19: $(RTX_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS_RTX) -o rtx.bin $(ASM) $(RTX_ASM) $(RTX_OBJS) 
	$(OBJCPY) --output-format=srec rtx.bin rtx.s19
	$(OBJDUMP) -xdC rtx.bin > rtx.lst

rtx_loader.s19: $(RTX_LOADER_OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS_RTX_LOADER) -o rtx_loader.bin $(ASM) $(RTX_LOADER_OBJS) 
	$(OBJCPY) --output-format=srec rtx_loader.bin rtx_loader.s19
	$(OBJDUMP) -xdC rtx_loader.bin > rtx_loader.lst

rtx_test_dummy.s19: $(RTX_TEST_DUMMY_OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS_RTX_TEST) -o rtx_test_dummy.bin $(ASM) $(RTX_TEST_DUMMY_OBJS) 
	$(OBJCPY) --output-format=srec rtx_test_dummy.bin rtx_test_dummy.s19
	$(OBJDUMP) -xdC rtx_test_dummy.bin > rtx_test_dummy.lst

stevee_test_dummy.s19: $(OST_TESTS_OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS_RTX_TEST) -o stevee_tests_dummy.bin $(ASM) $(OST_TESTS_OBJS) 
	$(OBJCPY) --output-format=srec stevee_tests_dummy.bin stevee_tests_dummy.s19
	$(OBJDUMP) -xdC stevee_tests_dummy.bin > stevee_tests_dummy.lst


m2.s19: rtx_test_dummy.s19 rtx.s19 
	python merge.py m2.s19 rtx.s19 rtx_test_dummy.s19 
	chmod u+x m2.s19

stevee2.s19: stevee_test_dummy.s19 rtx.s19 
	python merge.py stevee2.s19 rtx.s19 stevee_tests_dummy.s19 
	chmod u+x stevee2.s19

stevee_dummy.s19: stevee2.s19 rtx_loader.s19 
	python merge.py stevee_dummy.s19 stevee2.s19 rtx_loader.s19 
	chmod u+x stevee_dummy.s19

mdummy.s19: m2.s19 rtx_loader.s19
	python merge.py mdummy.s19 m2.s19 rtx_loader.s19 
	chmod u+x mdummy.s19

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@ 

.PHONY: clean
clean:
	rm -f *.bin *.s19 *.o *.map *.lst 
	rm -f ../*/*.o

sense:
	@echo "Make does make sense now!!!"
