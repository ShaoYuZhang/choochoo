#include "IoServer.h"
#include <ts7200.h>
#include <util.h>
#include <syscall.h>
#include <NameServer.h>

#define PUTC 1
#define GETC 2

typedef struct IOMessage {
  char type;
  char data;
} IOMessage;

char GetcCOM2(const int tid) {
  IOMessage msg;
  msg.type = GETC;
  Send(tid, (char*)&msg, sizeof(IOMessage), &(msg.data), 1);
  return msg.data ;
}

void PutcCOM2(const int tid, const char c) {
  IOMessage msg;
  msg.type = PUTC;
  msg.data = c;
  Send(tid, (char*)&msg, sizeof(IOMessage), NULL, 0);
}

void com2notifier_task() {
  int parent = MyParentsTid();

  for (;;) {
    // Enable com2 interrupt.
    bwprintf(COM1, "***awaitEvent\n");
    AwaitEvent(INT_UART2);
    bwprintf(COM1, "***woke up\n");
    Send(parent, (char*)NULL, 0, (char*)NULL, 0);
  }
}

void serv_putc(int* txempty, int* cts, char c);

void ioserver_task() {
  //char name[] = IOSERVER_NAME;
  //RegisterAs(name);

  int uartbase = UART_BASE(COM2);

  bwputstr(COM1, "----------------------------------------\n");
  bwputstr(COM1, "starting io server\n");
  VMEM(uartbase + UART_CTLR_OFFSET) |= TIEN_MASK | RIEN_MASK | MSIEN_MASK;
  VMEM(uartbase + UART_INTR_OFFSET) = 1;

  int com2 = Create(HIGHEST_PRIORITY, com2notifier_task);

  int txempty = VMEM(uartbase + UART_FLAG_OFFSET) & TXFE_MASK;
  IOMessage msg;
  int cts = 1;
  int tid = -1;
  int wait_send_tid = -1;

  for (;;) {
    bwputstr(COM1, "                  Server waiting\n");
    Receive(&tid, (char*)&msg, sizeof(IOMessage));

    if (tid == com2) {
      bwputstr(COM1, "serving notifier\n");
      int uart_isr = VMEM(uartbase + UART_INTR_OFFSET);
      int uart_flag = VMEM(uartbase + UART_FLAG_OFFSET);

      // An item can be received
      if (uart_isr & UARTRXINTR) {
        bwputstr(COM1, "an item can be received.\n");

        //ASSERT(uart_flag & RXFF_MASK, "incoming register empty");
        //ASSERT(FALSE, "not receiving..");
        // TODO, pick the guy wa
        if (wait_send_tid != -1) {
          char c = VMEM(uartbase + UART_DATA_OFFSET) & DATA_MASK;
          Reply(wait_send_tid, &c, 1);
          wait_send_tid = -1;
        }
      }

      // Can send stuff
      if (uart_isr & UARTTXINTR) {
        bwputstr(COM1, "out going buffer empty...\n");
        txempty = 1;
        VMEM(uartbase + UART_CTLR_OFFSET) &= ~TIEN_MASK;
      }

      // Control stuff, CTS
      if (uart_isr & UARTMSINTR) {
        bwputstr(COM1, "ms interrupt full...\n");
        cts = uart_flag & CTS_MASK;
        VMEM(uartbase + UART_INTR_OFFSET) = 1; // clear ms interrupt in hardware
      }
      //serv_putc(&txempty, &cts, 'x');

      Reply(tid, (char*)1, 0);
    }
    else if (msg.type == PUTC) {
      bwputstr(COM1, "PUTC client\n");
      serv_putc(&txempty, &cts, msg.data);
      Reply(tid, (char*)1, 0);
    }
    else if (msg.type == GETC) {
      bwputstr(COM1, "GETC client\n");
      int uart_isr = VMEM(uartbase + UART_INTR_OFFSET);
      if (uart_isr & UARTRXINTR) {
        char c = VMEM(uartbase + UART_DATA_OFFSET) & DATA_MASK;
        Reply(tid, &c, 1);
      } else {
        bwputstr(COM1, "wait .. for input.\n");
        wait_send_tid = tid;
      }
    }
  }
}

void serv_putc(int* txempty, int* cts, char c) {
  int uartbase = UART_BASE(COM2);
  if (*txempty && *cts) {

    bwputstr(COM1, "transferign.....\n");
    VMEM(uartbase + UART_DATA_OFFSET) = c;
    VMEM(uartbase + UART_CTLR_OFFSET) |= TIEN_MASK; // enable tx interrupt
    VMEM(uartbase + UART_INTR_OFFSET) = 1; // clear ms interrupt in hardware
    *cts = 1;
    *txempty = VMEM(uartbase + UART_CTLR_OFFSET) & TXFE_MASK;
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


