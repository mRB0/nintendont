#include <p18f4620.h>

#include "serial.h"
#include "interrupts.h"
#include "simpleplayer.h"

void low_isr(void);
void high_isr(void);

volatile INTERRUPTS _interrupts;

// set up interrupt vectors
#pragma code low_vector=0x18
void low_vector_interrupt(void)
{
	_asm GOTO low_isr _endasm
}
#pragma code

#pragma code high_vector=0x08
void high_vector_interrupt(void)
{
	_asm GOTO high_isr _endasm
}
#pragma code

/*
 * ISRs
 */
#pragma interruptlow low_isr isrlow_tmp //nosave=TABLAT,TBLPTRL,TBLPTRH //save=PRODH,PRODL //save=section("MATH_DATA"),section(".tmpdata")
void low_isr(void)
{
	/* no interrupts */
}

#pragma interrupt high_isr isrhigh_tmp //save=PRODH,PRODL,INDF0,POSTDEC0,POSTINC0,PREINC0,PLUSW0,section("MATH_DATA"),section(".tmpdata")
void high_isr(void)
{
	if (PIE1bits.RCIE && PIR1bits.RCIF)
	{
		serial_ISR();
	}
	
	if (PIE1bits.TMR1IE && PIR1bits.TMR1IF)
	{
		timer1_isr();
	}
}

/*
 * Helper functions
 */

void ISR_init(void)
{
	_interrupts.all = 0;
	RCONbits.IPEN = 1;
}

