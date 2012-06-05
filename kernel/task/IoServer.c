#include "IoServer.h"
#include <ts7200.h>
#include <util.h>
#include <syscall.h>
#include <NameServer.h>

char Getc(const int channel) {
  if (channel == COM1) {
  } else {
    ASSERT(FALSE, "Not implemented.");
  }
  return '\0';
}

void Putc(const int channel, const char c) {
  if (channel == COM1) {

  } else {
    ASSERT(FALSE, "Not implemented.");
  }
}

void com1notifier_task() {
}

void com2notifier_task() {
  int parent = MyParentsTid();

  bwprintf(COM1, "in notifier\n");
  // Enable com2 interrupt
  // This creates an interrupt.
  VMEM(VIC2 + INT_ENABLE) = 1 << (INT_UART2 & 31);

  for (;;) {
    bwprintf(COM1, "**awaitEvent\n");
    AwaitEvent(INT_UART2);
    bwprintf(COM1, "***woke up\n");
    Send(parent, (char*)NULL, 0, (char*)NULL, 0);
  }
}

void serv_putc(int* txempty, int* cts);

void ioserver_task() {
  //char name[] = IOSERVER_NAME;
  //RegisterAs(name);

  int uartbase = UART_BASE(COM2);

  bwputstr(COM1, "----------------------------------------\n");
  bwputstr(COM1, "starting io server\n");
  VMEM(uartbase + UART_CTLR_OFFSET) |= TIEN_MASK | RIEN_MASK | MSIEN_MASK;
  VMEM(uartbase + UART_INTR_OFFSET) = 1;

  int com1 = Create(HIGHEST_PRIORITY, com2notifier_task);

  int txempty = VMEM(uartbase + UART_FLAG_OFFSET) & TXFE_MASK;
  int cts = 1;

  int msgBuff = -1;
  int tid = -1;
  for (;;) {
    if (tid == com1) {
      bwputstr(COM1, "serving notifier\n");
      int uart_isr = VMEM(uartbase + UART_INTR_OFFSET);
      int uart_flag = VMEM(uartbase + UART_FLAG_OFFSET);

      // incoming register full
      if (uart_isr & UARTRXINTR) {
        bwputstr(COM1, "crash ...\n");
        *(char*)(1) = 10;
        for(;;) ;
        //ASSERT(uart_flag & RXFF_MASK, "incoming register empty");
        //ASSERT(FALSE, "not receiving..");
      }

      // outgoing register empty
      if (uart_isr & UARTTXINTR) {
        bwputstr(COM1, "out going buffer empty...\n");
        txempty = TRUE;
        VMEM(uartbase + UART_CTLR_OFFSET) &= ~TIEN_MASK;
      }

      if (uart_isr & UARTMSINTR) {
        bwputstr(COM1, "ms interrupt full...\n");
        cts = uart_flag & CTS_MASK;
        VMEM(uartbase + UART_INTR_OFFSET) = 1; // clear ms interrupt in hardware
      }

      serv_putc(&txempty, &cts);

      Reply(tid, (char*)1, 0);
    }
    else if (tid != -1) {
      bwputstr(COM1, "serving client\n");
      serv_putc(&txempty, &cts);
      Reply(tid, (char*)1, 0);
    }

    bwputstr(COM1, "                  Server waiting\n");
    int len = Receive(&tid, (char*)&msgBuff, 4);
  }
}

void serv_putc(int* txempty, int* cts) {
  int uartbase = UART_BASE(COM2);
  if (1) { //(txempty && cts) {
    if (!cts) {
      bwputstr(COM1, "EROROR____dont send.....\n");
    }
    bwputstr(COM1, "transferign.....\n");
    char c = 'x';
    VMEM(uartbase + UART_DATA_OFFSET) = c;
    VMEM(uartbase + UART_CTLR_OFFSET) |= TIEN_MASK; // enable tx interrupt
    VMEM(uartbase + UART_INTR_OFFSET) = 1; // clear ms interrupt in hardware
    cts = 1;
    txempty = VMEM(uartbase + UART_CTLR_OFFSET) & TXFE_MASK;
  }
}

int startIoServerTask() {
#if 0
  // COM1
  bwsetfifo(COM1, OFF);
  bwsetspeed(COM1, 2400);
	uart_stopbits(COM1, 2);
	uart_databits(COM1, 8);
	uart_parity(COM1, OFF);

	// COM2
	bwsetfifo(COM2, OFF);
	bwsetspeed(COM2, 115200);
	uart_stopbits(COM2, 1);
	uart_databits(COM2, 8);
	uart_parity(COM2, OFF);
#endif

  return Create(1, ioserver_task);
}


