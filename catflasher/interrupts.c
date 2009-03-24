#include <p18f4620.h>

#include "excitebike.h"

#include "interrupts.h"

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
	if (INTCON3bits.INT1IF)
	{
		LATDbits.LATD3 = 1;
		INTCON3bits.INT1IF = 0;
		LATDbits.LATD3 = 0;
	}

}

#pragma interrupt high_isr isrhigh_tmp //save=PRODH,PRODL,INDF0,POSTDEC0,POSTINC0,PREINC0,PLUSW0,section("MATH_DATA"),section(".tmpdata")
void high_isr(void)
{
	LATBbits.LATB2 = 0;
	LATBbits.LATB2 = 1;
	if (INTCONbits.INT0IF && !INTCON2bits.INTEDG0)
	{
		// read activated
		uint16_t addr;
		
		INTCONbits.INT0IF = 0;
		INTCON2bits.INTEDG0 = 1; // interrupt on read deactivate
		
		addr = ( (uint16_t)(PORTD&0x3f) << 8) | (uint16_t)PORTC;
		
		TRISA = 0; // data output
		LATA = excitebike[addr];
		//LATAbits.LATA7 = 0;
		//LATAbits.LATA7 = 1;
		//LATAbits.LATA7 = 0;
	}
	else if (INTCONbits.INT0IF && INTCON2bits.INTEDG0)
	{
		// read deactivated
		INTCONbits.INT0IF = 0;
		INTCON2bits.INTEDG0 = 0; // interrupt on read activate
		TRISA = 1;
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

