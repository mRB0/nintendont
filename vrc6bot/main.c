/* lol */

#include <p18f4620.h>

#include <delays.h>

#include "interrupts.h"
#include "ports.h"

void set_addr(uint16_t addr)
{
	LATA = addr & 0xff;
	LATBbits.LATB4 = (addr & 0x40) >> 6;
	
	LATB = ((addr >> (uint16_t)8) & 0x0f) | (LATB & 0xf0);
	LATC = ((addr >> (uint16_t)12) & 0x0f) | (LATC & 0xf0);
	
	Nop();
	Nop();
	
}

uint8_t port_read(uint16_t addr)
{
	uint8_t data;
	
	set_addr(addr);
	TRIS_DATA = TRIS_INPUT;
	
	LAT_OE = 0;
	Delay10TCYx(100);
	
	data = PORT_DATA;
	LAT_OE = 1;
	
	return data;
}
	
void port_write(uint16_t addr, uint8_t data)
{
	set_addr(addr);
	TRIS_DATA = TRIS_OUTPUT;
	LAT_DATA = data;
	LAT_WE = 0;
	LAT_WE = 1;
}
	

/*
 * The SPC module is located at 0x00 .. 0x03, with LAT_SPC_CE low.
 * On the SNES it is located at 0x2140 .. 0x2143, which are the same ports.
 *
 * So to communicate to SPC:
 *   
 */
void spc_init(void)
{
	LAT_SPC_RESET = 0; // hold in reset
	LAT_SPC_CE = 1;
	
	TRIS_SPC_RESET = 0;
	TRIS_SPC_CE = 0;
}

void system_init(void)
{
	// 8 MHz: IRCF2:0 = 0b111
	OSCCONbits.IRCF0 = 1;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF2 = 1;
	OSCCONbits.IDLEN = 1;
	
	// x4: 32 MHz
	OSCTUNEbits.PLLEN = 0;
	
	// interrupts on or off?
	ISR_disable();
	
	// I/O ports
	
	ADCON1 = 0x0f; // digital I/O
	
	
	TRISA = 0;
	TRISBbits.TRISB4 = 0; // this is address line A6, because RA6 is used for clock output.
	TRISB = 0;
	TRISC = 0;
	
	TRIS_DATA = TRIS_INPUT;
	
	TRIS_VRC6_CE = 0;
	LAT_VRC6_CE = 1;
	
	LAT_WE = 1;
	LAT_OE = 1;
	TRIS_WE = 0;
	TRIS_OE = 0;
	
	spc_init();
	
	Delay100TCYx(255);
	
}

void tester(void)
{
	int a = 5;
	tester();
}

void spc_test(void)
{
	uint8_t data;
	
	LAT_SPC_RESET = 1;
	
	// wait for 0xaa
	
	LAT_SPC_CE = 0;
	set_addr(0x0);
	TRIS_DATA = TRIS_INPUT;
	
	do
	{
		//data = port_read(0x0);
		
		Delay10TCYx(255);
		
		LAT_OE = 0;
		Delay10TCYx(255);
		data = PORT_DATA;
		LAT_OE = 1;
		
		Nop();
		Nop();
	} while(data != 0xaa);
	
	// wait for 0xbb
	
	do
	{
		data = port_read(0x1);
		
		Nop();
		Nop();
	} while(data != 0xbb);
	
	// write any non-zero value to 0x2141
	port_write(0x01, 0x55);
	
	// dest addr
	port_write(0x02, 0x00);
	port_write(0x03, 0x00);
	// write 0xcc and wait for 0xcc
	
	port_write(0x00, 0xcc);
	
	do
	{
		data = port_read(0x00);
		
		Nop();
		Nop();
	} while(data != 0xcc);
	
	
	LAT_SPC_CE = 1;
	
}

void main(void)
{
	uint8_t m2 = 0;
	
	uint8_t duty = 0x00;
	
	uint8_t test = 0xAABB;
	int16_t test2 = test / 4;
	
	system_init();
	
	spc_test();
	
	set_addr(0x9000);
	LATD = 0x3f;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;
	
	set_addr(0x9001);
	LATD = 0xff;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;

	set_addr(0x9002);
	LATD = 0x06;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;
	
	set_addr(0xa000);
	LATD = 0x7f;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;
	
	set_addr(0xa001);
	LATD = 0x33;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;
	
	set_addr(0xa002);
	LATD = 0x87;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;
	
	set_addr(0xb000);
	LATD = 0x3f;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;
	
	set_addr(0xb001);
	LATD = 0xff;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;
	
	set_addr(0xb002);
	LATD = 0x0f;
	LATEbits.LATE1 = 0;
	LATEbits.LATE2 = 0;
	Nop();
	LATEbits.LATE2 = 1;
	LATEbits.LATE1 = 1;
	
	for(;;)
	{
		duty = (duty + 1) % 0x8;
		
		
		// set duty
		
		set_addr(0x9000);
		LATD = 0x0f | (duty << 4);
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
		
		//duty = (duty + 4) % 0x8;
		
		set_addr(0xa000);
		LATD = 0x0f | (duty << 4);
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
		
		//duty = (duty + 4) % 0x8;
		
		// set pitches
		
		set_addr(0xa001);
		LATD = 0x72;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
		
		set_addr(0xa002);
		LATD = 0x82;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
		
		set_addr(0x9001);
		LATD = 0xd6;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
	
		set_addr(0x9002);
		LATD = 0x81;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
		
		//Delay10KTCYx(50);
		
		set_addr(0xa001);
		LATD = 0xe7;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
	
		set_addr(0xa002);
		LATD = 0x82;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
		
		set_addr(0x9001);
		LATD = 0x72;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
	
		set_addr(0x9002);
		LATD = 0x82;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
		
		//Delay10KTCYx(50);
		
		set_addr(0xa001);
		LATD = 0xa7;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
	
		set_addr(0xa002);
		LATD = 0x83;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;

		set_addr(0x9001);
		LATD = 0xe7;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
	
		set_addr(0x9002);
		LATD = 0x82;
		LATEbits.LATE1 = 0;
		LATEbits.LATE2 = 0;
		LATEbits.LATE2 = 1;
		LATEbits.LATE1 = 1;
		
		
		//Delay10KTCYx(50);
		
		
	}
}
