#include "IoServer.h"
#include <ts7200.h>
#include <util.h>
#include <syscall.h>
#include <NameServer.h>
#include <IoBuffer.h>

#define PUTC 1
#define GETC 2

char Getc(const int tid) {
  IOMessage msg;
  msg.type = GETC;
  Send(tid, (char*)&msg, sizeof(IOMessage), &(msg.data), 1);
  return msg.data ;
}

void Putc(const int tid, const char c) {
  IOMessage msg;
  msg.type = PUTC;
  msg.data = c;
  Send(tid, (char*)&msg, sizeof(IOMessage), NULL, 0);
}

void com1rx_task() {
  int parent = MyParentsTid();

  // Enables UART1RXINTR1
  VMEM(UART1_BASE + UART_CTLR_OFFSET) |= RIEN_MASK;
  VMEM(VIC1 + INT_ENABLE) = 1 << (UART1RXINTR1 & 31);

  IOMessage msg;
  for (;;) {
    msg.data = (char)AwaitEvent(UART2TXINTR2);
    Send(parent, (char *)&msg, sizeof(IOMessage), (char *)NULL, 0);
  }
}

void com2rx_task() {
  int parent = MyParentsTid();

  // Enables UART2RXINTR2
  VMEM(UART2_BASE + UART_CTLR_OFFSET) |= RIEN_MASK;
  VMEM(VIC1 + INT_ENABLE) = 1 << (UART2RXINTR2 & 31);

  IOMessage msg;
  for (;;) {
    msg.data = (char)AwaitEvent(UART2RXINTR2);
    Send(parent, (char *)&msg, sizeof(IOMessage), (char *)NULL, 0);
  }
}

void com1tx_task() {
  int parent = MyParentsTid();

  // Enables UART1TXINTR1
  VMEM(UART1_BASE + UART_CTLR_OFFSET) |= TIEN_MASK;
  VMEM(VIC1 + INT_ENABLE) = 1 << (UART1TXINTR1 & 31);

  for (;;) {
    AwaitEvent(UART1TXINTR1);
    Send(parent, (char *)NULL, 0, (char *)NULL, 0);
  }
}

void com2tx_task() {
  int parent = MyParentsTid();

  // Enables UART2TXINTR2
  VMEM(UART2_BASE + UART_CTLR_OFFSET) |= TIEN_MASK;
  VMEM(VIC1 + INT_ENABLE) = 1 << (UART2TXINTR2 & 31);

  for (;;) {
    AwaitEvent(UART2TXINTR2);
    Send(parent, (char *)NULL, 0, (char *)NULL, 0);
  }
}

void com1ms_task() {
  int parent = MyParentsTid();

  // Enables INT_UART1
  VMEM(UART1_BASE + UART_CTLR_OFFSET) |= MSIEN_MASK;
  VMEM(VIC2 + INT_ENABLE) = 1 << (INT_UART1 & 31);

  IOMessage msg;
  for (;;) {
    msg.data = AwaitEvent(INT_UART1); // this is techinically waiting for MS change
    Send(parent, (char *)&msg, sizeof(IOMessage), (char *)NULL, 0);
  }
}

void ioserver_com1_task() {
  char name[] = IOSERVERCOM1_NAME;
  RegisterAs(name);

  IOBuffer com1In;
  IOBuffer com1Out;

  com1In.bufferHead = 0;
  com1In.bufferTail = 0;
  com1Out.bufferHead = 0;
  com1Out.bufferTail = 0;

  int com1InputWaitTid = -1;

  int uartbase = UART_BASE(COM1);
  int uart_isr = uartbase + UART_INTR_OFFSET;
  int uart_flag = uartbase + UART_FLAG_OFFSET;

  VMEM(uart_isr) = 1; // clear msintr
  VMEM(UART1_BASE + UART_DATA_OFFSET); // clear rxintr

  int txempty = VMEM(uart_flag) & TXFE_MASK;
  int cts = VMEM(uart_flag) & CTS_MASK;

  IOMessage msg;
  int tid = -1;

  int com1_rx_id = Create(HIGHEST_PRIORITY, com1rx_task);
  int com1_tx_id = Create(HIGHEST_PRIORITY, com1tx_task);
  int com1_ms_id = Create(HIGHEST_PRIORITY, com1ms_task);

  for (;;) {
    Receive(&tid, (char*)&msg, sizeof(IOMessage));

    if (tid == com1_rx_id) {
      Reply(tid, (char*)1, 0);

      char c = msg.data;
      if (com1InputWaitTid!= -1) {
        Reply(com1InputWaitTid, &c, 1);
        com1InputWaitTid= -1;
      } else {
        add_to_buffer( &com1In, c);
      }
    }
    else if (tid == com1_tx_id) {
      Reply(tid, (char*)1, 0);
      txempty = 1;
    }
    else if (tid == com1_ms_id) {
      Reply(tid, (char*)1, 0);
      char flag = msg.data;
      cts = flag & CTS_MASK;
    }
    else if (msg.type == PUTC) {
      add_to_buffer( &com1Out, msg.data);
      Reply(tid, (char*)1, 0);
    }
    else if (msg.type == GETC) {
      if (!buffer_empty(&com1In)) {
        char c = remove_from_buffer(&com1In);
        Reply(tid, &c, 1);
      } else {
        com1InputWaitTid = tid;
      }
    }

    if (!buffer_empty(&com1Out) && txempty && cts) {
      VMEM(uartbase + UART_DATA_OFFSET) = remove_from_buffer(&com1Out);
      VMEM(uartbase + UART_CTLR_OFFSET) |= TIEN_MASK; // enable tx interrupt
      txempty = 0;
      cts = VMEM(uart_flag) & CTS_MASK;
    }
  }
}

void ioserver_com2_task() {
  char name[] = IOSERVERCOM2_NAME;
  RegisterAs(name);

  IOBuffer com2In;
  IOBuffer com2Out;

  com2In.bufferHead = 0;
  com2In.bufferTail = 0;
  com2Out.bufferHead = 0;
  com2Out.bufferTail = 0;

  int com2InputWaitTid = -1;

  int uartbase = UART_BASE(COM2);
  int uart_isr = uartbase + UART_INTR_OFFSET;
  int uart_flag = uartbase + UART_FLAG_OFFSET;

  VMEM(uart_isr) = 1; // clear msintr
  VMEM(UART2_BASE + UART_DATA_OFFSET); // clear rxintr

  int txempty = VMEM(uart_flag) & TXFE_MASK;

  IOMessage msg;
  int tid = -1;

  int com2_rx_id = Create(HIGHEST_PRIORITY, com2rx_task);
  int com2_tx_id = Create(HIGHEST_PRIORITY, com2tx_task);

  for (;;) {
    Receive(&tid, (char*)&msg, sizeof(IOMessage));

    if (tid == com2_rx_id) {
      Reply(tid, (char*)1, 0);

      char c = msg.data;
      if (com2InputWaitTid!= -1) {
        Reply(com2InputWaitTid, &c, 1);
        com2InputWaitTid= -1;
      } else {
        add_to_buffer( &com2In, c);
      }
    }
    else if (tid == com2_tx_id) {
      Reply(tid, (char*)1, 0);
      txempty = 1;
    }
    else if (msg.type == PUTC) {
      add_to_buffer( &com2Out, msg.data);
      Reply(tid, (char*)1, 0);
    }
    else if (msg.type == GETC) {
      if (!buffer_empty(&com2In)) {
        char c = remove_from_buffer(&com2In);
        Reply(tid, &c, 1);
      } else {
        com2InputWaitTid = tid;
      }
    }
    if (!buffer_empty(&com2Out) && txempty ) {
      VMEM(uartbase + UART_DATA_OFFSET) = remove_from_buffer(&com2Out);
      VMEM(uartbase + UART_CTLR_OFFSET) |= TIEN_MASK; // enable tx interrupt
      txempty = 0;
    }
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

  return Create(1, ioserver_com1_task);
}
