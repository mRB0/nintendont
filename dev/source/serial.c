#include <p18f4620.h>

#include <usart.h>

#include "interrupts.h"

#include "serial.h"


unsigned char _serial_bufdata[RXBUF_LEN];
CIRCBUF _rxbuf;


void serial_init(void)
{
	// set up circbuf
	_rxbuf.bufdata = _serial_bufdata;
	_rxbuf.bufsize = RXBUF_LEN;
	_rxbuf.szmask = RXBUF_LENMASK; // 0x0f;  szmask is always 1 less than bufsize
	_rxbuf.start = _rxbuf.writepos = 0;
	
	
	// open at 57600 or 115200 baud
	OpenUSART(USART_TX_INT_OFF &
	          USART_RX_INT_ON &
	          USART_ASYNCH_MODE &
	          USART_EIGHT_BIT &
	          USART_CONT_RX &
	          USART_BRGH_HIGH,
	          68); // 138 = 57600, 68 = 115200
	
	IPR1bits.RCIP = 0;
	
	BAUDCONbits.BRG16 = 1;
}

#pragma tmpdata isrhigh_tmp
void serial_ISR(void)
{
	unsigned char rxdata;
	
	_interrupts.rx = 1;
	rxdata = RCREG;
	
	Nop();
	Nop();
	
	if (!CIRCBUF_FULL(_rxbuf))
	{
		Nop();
		Nop();
		CIRCBUF_PUSHCHAR_INLINE(_rxbuf, rxdata);
	}
	
	// clear errors
	if (RCSTAbits.OERR)
	{
		RCSTAbits.CREN = 0;
		RCSTAbits.CREN = 1;
	}
	
}
