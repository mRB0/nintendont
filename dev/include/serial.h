#ifndef SERIAL_H__
#define SERIAL_H__

#include "circbuf.h"

#define RXBUF_LEN 4     // should always be a power of 2 (2, 4, 8, 16, ...)
#define RXBUF_LENMASK (RXBUF_LEN-1) // (eg. len 16 = mask 0x0f) should always be one less than LEN

extern unsigned char _serial_bufdata[];
extern CIRCBUF _rxbuf;

void serial_init(void);
void serial_ISR(void);

#endif
