/* lol */

#include <p18f4620.h>

#include <delays.h>

#include "interrupts.h"
#include "ports.h"

void system_init(void)
{
	// 8 MHz: IRCF2:0 = 0b111
	OSCCONbits.IRCF0 = 0;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF2 = 1;
	OSCCONbits.IDLEN = 1;
	
	// x4: 32 MHz
	OSCTUNEbits.PLLEN = 1;
	
	// interrupts on or off?
	ISR_disable();
	
	// I/O ports
	
	ADCON1 = 0x0f; // digital I/O
	
	
	TRISA = 0;
	TRISB = 0;
	TRISC = 0;
	TRISD = 0;
	LATEbits.LATE0 = 1;
	LATEbits.LATE1 = 1;
	LATEbits.LATE2 = 1;
	TRISE = 0;
	
	TRIS_M2 = 0; // output
}

void tester(void)
{
	int a = 5;
	tester();
}

void set_addr(uint16_t addr)
{
	LATA = addr & 0xff;
	LATB = ((addr >> (uint16_t)8) & 0x0f) | (LATB & 0xf0);
	LATC = ((addr >> (uint16_t)12) & 0x0f) | (LATC & 0xf0);
	
	Nop();
	Nop();
	
}

void main(void)
{
	uint8_t m2 = 0;
	
	uint8_t duty = 0x00;
	
	uint8_t test = 0xAABB;
	uint16_t test2 = test * 127;
	
	system_init();
	
	LATEbits.LATE1 = 0;
	
	set_addr(0x9000);
	LATD = 0x3f;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;
	
	set_addr(0x9001);
	LATD = 0xff;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;

	set_addr(0x9002);
	LATD = 0x06;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;
	
	set_addr(0xa000);
	LATD = 0x7f;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;
	
	set_addr(0xa001);
	LATD = 0x33;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;
	
	set_addr(0xa002);
	LATD = 0x87;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;
	
	set_addr(0xb000);
	LATD = 0x3f;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;
	
	set_addr(0xb001);
	LATD = 0xff;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;
	
	set_addr(0xb002);
	LATD = 0x0f;
	LATEbits.LATE2 = 0;
	LATEbits.LATE2 = 1;
	
	LATEbits.LATE1 = 1;
	
	for(;;)
	{
		duty = (duty + 1) % 0x8;
		
		LATEbits.LATE1 = 0;
		
		// set duty
		
		set_addr(0x9000);
		LATD = 0x0f | (duty << 4);
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
		
		//duty = (duty + 4) % 0x8;
		
		set_addr(0xa000);
		LATD = 0x0f | (duty << 4);
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
		
		//duty = (duty + 4) % 0x8;
		
		// set pitches
		
		set_addr(0xa001);
		LATD = 0x72;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
		
		set_addr(0xa002);
		LATD = 0x82;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
		
		set_addr(0x9001);
		LATD = 0xd6;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
	
		set_addr(0x9002);
		LATD = 0x81;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
		
		Delay10KTCYx(50);
		
		set_addr(0xa001);
		LATD = 0xe7;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
	
		set_addr(0xa002);
		LATD = 0x82;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
		
		set_addr(0x9001);
		LATD = 0x72;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
	
		set_addr(0x9002);
		LATD = 0x82;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
		
		Delay10KTCYx(50);
		
		set_addr(0xa001);
		LATD = 0xa7;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
	
		set_addr(0xa002);
		LATD = 0x83;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;

		set_addr(0x9001);
		LATD = 0xe7;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
	
		set_addr(0x9002);
		LATD = 0x82;
		LATEbits.LATE2 = 0;
		Delay1KTCYx(1);
		LATEbits.LATE2 = 1;
		
		LATEbits.LATE1 = 1;
		
		Delay10KTCYx(50);
		
		
	}
}
