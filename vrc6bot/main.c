/* lol */

#include <p18f4620.h>

#include "interrupts.h"

void system_init(void)
{
	// 8 MHz
	OSCCONbits.IRCF0 = 1;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF2 = 1;
	OSCCONbits.IDLEN = 1;
	
	// x4: 32 MHz
	OSCTUNEbits.PLLEN = 1;
	
	// interrupts on or off?
	ISR_disable();
	
	// I/O ports
	
	ADCON1 = 0x0f; // digital I/O
}

void tester(){
	int a = 5;
	tester();
}

void main(void)
{
	uint8_t test = 0xAABB;
	uint16_t test2 = test * 127;

	while(1) {}
	system_init();
	
	while(1);
}
