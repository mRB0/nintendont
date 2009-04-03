/* lol */

#include <p18f4620.h>

#include <delays.h>

#include "interrupts.h"
#include "ports.h"

void set_addr(uint24_t addr)
{
	LATA = addr & 0xff;
	//LATAbits.LATA0 = (addr & 0x02) >> 1;
	//LATAbits.LATA1 = addr & 0x01;
	
	LATBbits.LATB4 = (addr & 0x40) >> 6;
	
	LATB = ((addr >> (uint16_t)8) & 0x0f) | (LATB & 0xf0);
	LATC = ((addr >> (uint16_t)12) & 0x0f) | (LATC & 0xf0);
	
	LATCbits.LATC5 = (addr >> 16) & 0x1;
	
	// XXX temporary, for testing
	LATCbits.LATC6 = (addr >> 17) & 0x1;
	
	Nop();
	Nop();
	
}

uint8_t port_read(uint24_t addr)
{
	uint8_t data;
	
	set_addr(addr);
	TRIS_DATA = TRIS_INPUT;
	
	LAT_OE = 0;
	//Delay10TCYx(100);
	
	data = PORT_DATA;
	LAT_OE = 1;
	
	return data;
}
	
void port_write(uint24_t addr, uint8_t data)
{
	set_addr(addr);
	TRIS_DATA = TRIS_OUTPUT;
	LAT_DATA = data;
	LAT_WE = 0;
	
	// these delays are required for vrc6 when
	// Fosc = 32 MHz and vrc6 = 2 MHz :(
	Nop();
	Nop();
	
	//Delay10TCYx(1);
	
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
	//LAT_SPC_RESET = 0; // hold in reset
	DEACTIVATE_SPC();
	
	//TRIS_SPC_RESET = 0;
	TRIS_SPC_CE = 0;
	
	// reset (A7 is SPC_RESET, not inverted, active whcn /CE)
	set_addr(0x80);
	ACTIVATE_SPC();
	
	Delay1KTCYx(100);
	
	DEACTIVATE_SPC();
}

void vrc6_init(void)
{
	uint16_t i;
	
	// delay some time to allow oscillator to settle
	/*for(i=0; i<10; i++)
	{
		Delay100TCYx(255);
		Delay100TCYx(255);
		Delay100TCYx(255);
	}*/
	
	TRIS_VRC6_CE = 0;
	ACTIVATE_VRC6();
	
	// init vrc6 mapping stuff
	
	port_write(0x9003, 0x00);
	
	port_write(0x8000, 0x00);
	port_write(0xc000, 0x00);
	
	port_write(0xb003, 0x08);
	port_write(0xf000, 0x00);
	port_write(0xf001, 0x00);
	port_write(0xf002, 0x00);

	//DEACTIVATE_VRC6();
	
	// init vrc6 sound
	
	//ACTIVATE_VRC6();
	
	port_write(0x9000, 0x7f);
	port_write(0x9001, 0x00);
	port_write(0x9002, 0x00);
	
	port_write(0xa000, 0x7f);
	port_write(0xa001, 0x00);
	port_write(0xa002, 0x00);

	port_write(0xb000, 0x3f);
	port_write(0xb001, 0x00);
	port_write(0xb002, 0x00);
	
	DEACTIVATE_VRC6();
	
}

void flash_test(void)
{
	uint8_t sig_mfr, sig_dev;
	ACTIVATE_FL0();
	
	port_write(0x0, 0x90);
	sig_mfr = port_read(0x0);
	
	port_write(0x0, 0x90);
	sig_dev = port_read(0x1);
	
	DEACTIVATE_FL0();
	
	//for(;;);
}

void system_init(void)
{
	// 8 MHz: IRCF2:0 = 0b111
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
	
	
	TRISA = 0;
	//TRISBbits.TRISB4 = 0; // this is address line A6, because RA6 is used for clock output.
	TRISB = 0;
	TRISC = 0;
	
	TRIS_DATA = TRIS_INPUT;
	
	DEACTIVATE_FL0();
	TRIS_FL0_CE = 0;
	
	LAT_WE = 1;
	LAT_OE = 1;
	TRIS_WE = 0;
	TRIS_OE = 0;
	
	spc_init();
	vrc6_init();
	
	
}

void tester(void)
{
	int a = 5;
	tester();
}

void spc_test(void)
{
	uint8_t data;
	
	//LAT_SPC_RESET = 1;
	
	// wait for 0xaa
	
	ACTIVATE_SPC();
	
	do
	{
		data = port_read(0x2140);
		
		Nop();
		Nop();
	} while(data != 0xaa);
	
	// wait for 0xbb
	
	do
	{
		data = port_read(0x2141);
		
		Nop();
		Nop();
	} while(data != 0xbb);
	
	// write any non-zero value to 0x2141
	port_write(0x2141, 0x55);
	
	// dest addr
	port_write(0x2142, 0x00);
	port_write(0x2143, 0x00);
	// write 0xcc and wait for 0xcc
	
	port_write(0x2140, 0xcc);
	
	do
	{
		data = port_read(0x2140);
		
		Nop();
		Nop();
	} while(data != 0xcc);
	
	
	DEACTIVATE_SPC();
	
}

void main(void)
{
	
	uint8_t duty = 0xff;
	
	system_init();
	
	//spc_test();
	//flash_test();
	
	for(;;)
	{
		duty = (duty + 1) % 0x8;
		
		// set duty
		
		ACTIVATE_VRC6();
		
		port_write(0x9000, 0x0f | (duty << 4));
		port_write(0xa000, 0x0f | (duty << 4));
		
		// set pitches
		
		port_write(0xa001, 0xa7);
		port_write(0xa002, 0x83);
		port_write(0x9001, 0xe7);
		port_write(0x9002, 0x82);
		
		Delay10KTCYx(75);
		
		port_write(0xa001, 0xe7);
		port_write(0xa002, 0x82);
		port_write(0x9001, 0x72);
		port_write(0x9002, 0x82);
		
		Delay10KTCYx(75);
		
		port_write(0xa001, 0x72);
		port_write(0xa002, 0x82);
		port_write(0x9001, 0xd6);
		port_write(0x9002, 0x81);

		DEACTIVATE_VRC6();
		
		Delay10KTCYx(50);
		
		
	}
}
